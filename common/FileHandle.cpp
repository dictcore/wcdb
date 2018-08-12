/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <WCDB/Assertion.hpp>
#include <WCDB/FileHandle.hpp>
#include <WCDB/Notifier.hpp>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace WCDB {

FileHandle::FileHandle(const std::string &path_)
: path(path_), m_fd(-1), m_mode(Mode::None), m_errorIgnorable(false)
{
}

FileHandle::FileHandle(FileHandle &&other)
: path(std::move(other.path)), m_fd(other.m_fd), m_mode(other.m_mode)
{
    other.m_fd = -1;
    other.m_mode = Mode::None;
}

FileHandle::~FileHandle()
{
    WCTRemedialAssert(!isOpened() || m_mode != Mode::OverWrite,
                      "Close should be call manually to sync file.",
                      ;);
    close();
}

FileHandle &FileHandle::operator=(FileHandle &&other)
{
    WCTInnerAssert(path == other.path);
    m_fd = std::move(other.m_fd);
    other.m_fd = -1;
    other.m_mode = Mode::None;
    return *this;
}

bool FileHandle::open(Mode mode)
{
    WCTInnerAssert(mode != Mode::None);
    WCTRemedialAssert(!isOpened(), "File already is opened", markAsMisuse("Duplicate open.");
                      return true;);
    switch (mode) {
    case Mode::OverWrite:
        m_fd = ::open(path.c_str(),
                      O_CREAT | O_WRONLY | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); //0x0644
        break;
    case Mode::ReadOnly:
        m_fd = ::open(path.c_str(), O_RDONLY);
        break;
    default:
        markAsMisuse("Invalid open mode.");
        return false;
    }
    if (m_fd == -1) {
        setThreadedError();
        return false;
    }
    m_mode = mode;
    return true;
}

bool FileHandle::isOpened() const
{
    return m_fd != -1;
}

void FileHandle::close()
{
    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

ssize_t FileHandle::size()
{
    WCTInnerAssert(isOpened());
    return (ssize_t) lseek(m_fd, 0, SEEK_END);
}

Data FileHandle::read(off_t offset, size_t size)
{
    WCTInnerAssert(isOpened());
    Data data(size);
    if (data.empty()) {
        return Data::emptyData();
    }
    ssize_t got;
    size_t prior = 0;
    unsigned char *buffer = data.buffer();
    do {
        got = pread(m_fd, buffer, size, offset);
        if (got == size) {
            break;
        }
        if (got < 0) {
            if (errno == EINTR) {
                got = 1;
                continue;
            }
            prior = 0;
            setThreadedError();
            break;
        } else if (got > 0) {
            size -= got;
            offset += got;
            prior += got;
            buffer = got + buffer;
        }
    } while (got > 0);
    if (got + prior == size) {
        return data;
    }
    Error error;
    error.setSystemCode(EIO, Error::Code::IOError);
    error.message = "Short read.";
    error.infos.set("Path", path);
    Notifier::shared()->notify(error);
    SharedThreadedErrorProne::setThreadedError(std::move(error));
    return data.subdata(got + prior);
}

bool FileHandle::write(off_t offset, const UnsafeData &unsafeData)
{
    WCTInnerAssert(isOpened());
    ssize_t wrote;
    ssize_t prior = 0;
    size_t size = unsafeData.size();
    const unsigned char *buffer = unsafeData.buffer();
    do {
        wrote = pwrite(m_fd, buffer, size, offset);
        if (wrote == size) {
            break;
        }
        if (wrote < 0) {
            if (errno == EINTR) {
                wrote = 1;
                continue;
            }
            setThreadedError();
            break;
        } else if (wrote > 0) {
            size -= wrote;
            offset += wrote;
            prior += wrote;
            buffer = wrote + buffer;
        }
    } while (wrote > 0);
    if (wrote + prior == size) {
        return true;
    }
    Error error;
    error.setSystemCode(EIO, Error::Code::IOError);
    error.message = "Short write.";
    error.infos.set("Path", path);
    Notifier::shared()->notify(error);
    SharedThreadedErrorProne::setThreadedError(std::move(error));
    return false;
}

MappedData FileHandle::map(off_t offset, size_t size)
{
    WCTRemedialAssert(m_mode == Mode::ReadOnly,
                      "Map is only supported in Readonly mode.",
                      return MappedData::emptyData(););
    WCTInnerAssert(size > 0);
    static int s_pagesize = getpagesize();
    int alignment = offset % s_pagesize;
    off_t roundedOffset = offset - alignment;
    size_t roundedSize = size + alignment;
    void *mapped = mmap(
    nullptr, roundedSize, PROT_READ, MAP_PRIVATE | MAP_NOEXTEND | MAP_NORESERVE, m_fd, roundedOffset);
    if (mapped == MAP_FAILED) {
        setThreadedError();
        return MappedData::emptyData();
    }
    return MappedData(reinterpret_cast<unsigned char *>(mapped), roundedSize).subdata(alignment, size);
}

void FileHandle::markErrorAsIgnorable(bool flag)
{
    m_errorIgnorable = flag;
}

void FileHandle::setThreadedError()
{
    Error error;
    if (m_errorIgnorable) {
        error.level = Error::Level::Warning;
    }
    error.setSystemCode(errno, Error::Code::IOError);
    error.message = strerror(errno);
    error.infos.set("Path", path);
    Notifier::shared()->notify(error);
    SharedThreadedErrorProne::setThreadedError(std::move(error));
}

void FileHandle::markAsMisuse(const char *message)
{
    Error error;
    if (m_errorIgnorable) {
        error.level = Error::Level::Warning;
    }
    error.setCode(Error::Code::Misuse);
    error.message = message;
    error.infos.set("Path", path);
    Notifier::shared()->notify(error);
    SharedThreadedErrorProne::setThreadedError(std::move(error));
}

} //namespace WCDB
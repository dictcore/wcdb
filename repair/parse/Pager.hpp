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

#ifndef Pager_hpp
#define Pager_hpp

#include <WCDB/Error.hpp>
#include <WCDB/FileHandle.hpp>
#include <WCDB/ThreadedErrorProne.hpp>
#include <WCDB/Wal.hpp>

namespace WCDB {

class Data;

namespace Repair {

class Pager : public SharedThreadedErrorProne {
#pragma mark - Initialize
public:
    Pager(const std::string &path);

    void setPageSize(int pageSize);
    void setReservedBytes(int reservedBytes);

    bool initialize();

    const std::string &getPath() const;

protected:
    FileHandle m_fileHandle;

#pragma mark - Page
public:
    int getPageCount() const;
    Data acquirePageData(int number);
    Data acquireData(off_t offset, size_t size);

    int getUsableSize() const;
    int getPageSize() const;
    int getReservedBytes() const;

protected:
    int m_pageSize;
    int m_reservedBytes;
    int m_pageCount;

#pragma mark - Wal
protected:
    Wal m_wal;
    bool m_walSanity;

#pragma mark - Error
public:
    void markAsCorrupted();

protected:
    void markAsNotADatabase();
    void markAsError(Error::Code code);
};

} //namespace Repair

} //namespace WCDB

#endif /* Pager_hpp */

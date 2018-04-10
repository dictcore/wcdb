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

#import <WCDB/Interface.h>
#import <WCDB/WCTBuiltinConfig.h>
#import <WCDB/WCTCore+Private.h>
#import <WCDB/WCTError+Private.h>
#import <WCDB/WCTUnsafeHandle+Private.h>

@implementation WCTDatabase

- (instancetype)initWithPath:(NSString *)path
{
    if (self = [super initWithDatabase:WCDB::Database::databaseWithPath(path.UTF8String)]) {
#if TARGET_OS_IPHONE
        _database->setConfig(WCTBuiltinConfig::fileProtection);
#endif //TARGET_OS_IPHONE
    }
    return self;
}

- (instancetype)initWithExistingTag:(WCTTag)tag
{
    return [super initWithDatabase:WCDB::Database::databaseWithExistingTag(tag)];
}

- (void)setTag:(WCTTag)tag
{
    _database->setTag(tag);
}

- (BOOL)canOpen
{
    return _database->canOpen();
}

- (BOOL)isOpened
{
    return _database->isOpened();
}

- (void)close
{
    _database->close(nullptr);
}

- (void)close:(WCTCloseBlock)onClosed
{
    std::function<void(void)> callback = nullptr;
    if (onClosed) {
        callback = [onClosed]() {
            onClosed();
        };
    }
    _database->close(callback);
}

- (BOOL)isBlockaded
{
    return _database->isBlockaded();
}

- (void)blockade
{
    _database->blockade();
}

- (bool)blockadeUntilDone:(WCTBlockadeBlock)onBlockaded
{
    return _database->blockadeUntilDone([onBlockaded, self](WCDB::Handle *handle) {
        onBlockaded([[WCTHandle alloc] initWithDatabase:_database andHandle:handle]);
    });
}

- (void)unblockade
{
    _database->unblockade();
}

- (WCTError *)error
{
    return [WCTError errorWithWCDBError:_database->getError()];
}

@end
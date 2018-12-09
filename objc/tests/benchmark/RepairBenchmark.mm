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

#import "Benchmark.h"

@interface RepairBenchmark : Benchmark

@property (nonatomic, readonly) double tolerablePercentageForFileSize;

@property (nonatomic, readonly) int fillStep;

@property (nonatomic, readonly) NSString* firstMaterial;

@property (nonatomic, readonly) NSString* lastMaterial;

@property (nonatomic, readonly) NSUInteger configForSizeToBackup;
@property (nonatomic, readonly) NSUInteger configForSizeToRepair;

@end

@implementation RepairBenchmark

- (void)setUp
{
    [super setUp];

    _configForSizeToBackup = 500 * 1024 * 1024; // 500MB
    _configForSizeToRepair = 100 * 1024 * 1024; // 100MB

    _tolerablePercentageForFileSize = 0.01f;

    _fillStep = 10000;

    _firstMaterial = [self.database.path stringByAppendingString:@"-first.material"];
    _lastMaterial = [self.database.path stringByAppendingString:@"-last.material"];

    [self.database removeConfigForName:WCTConfigNameCheckpoint];
}

- (BOOL)fillDatabase:(NSUInteger)expectedSize
{
    if ([self.database getFilesSize] > expectedSize * (1.0f + self.tolerablePercentageForFileSize)) {
        XCTAssertTrue([self.database removeFiles]);
    }

    NSMutableArray* objects = [NSMutableArray array];
    for (int i = 0; i < self.fillStep; ++i) {
        TestCaseObject* object = [[TestCaseObject alloc] init];
        object.isAutoIncrement = YES;
        object.content = [NSString randomString];
        [objects addObject:object];
    }

    __block NSString* currentTable = nil;
    int percentage = 0;
    for (NSUInteger size = [self.database getFilesSize]; size < expectedSize; size = [self.database getFilesSize]) {
        int gap = (double) size / expectedSize * 100 - percentage;
        if (gap >= 5) {
            percentage += gap;
            TestLog(@"Preparing %d%%", percentage);
        }

        if (![self.database runTransaction:^BOOL(WCTHandle* handle) {
                if (currentTable == nil
                    || [NSNumber randomBool]) {
                    currentTable = [NSString stringWithFormat:@"t_%@", [NSString randomString]];
                    if (![self.database createTableAndIndexes:currentTable withClass:TestCaseObject.class]) {
                        return NO;
                    }
                }
                return [self.database insertObjects:objects intoTable:currentTable];
            }]) {
            return NO;
        }

        if (![self.database execute:WCDB::StatementPragma().pragma(WCDB::Pragma::walCheckpoint()).to("TRUNCATE")]) {
            return NO;
        }
    }

    [self log:@"database size: %fMB", (double) [self.database getFilesSize] / 1024 / 1024];
    return YES;
}

- (void)test_backup
{
    [self
    measure:^{
        TestCaseAssertTrue([self.database backup]);
    }
    setUp:^{
        // 500MB
        TestCaseAssertTrue([self fillDatabase:self.configForSizeToBackup]);
    }
    tearDown:^{
        if ([self.fileManager fileExistsAtPath:self.firstMaterial]) {
            TestCaseAssertTrue([self.fileManager removeItemAtPath:self.firstMaterial error:nil]);
        }
        if ([self.fileManager fileExistsAtPath:self.lastMaterial]) {
            TestCaseAssertTrue([self.fileManager removeItemAtPath:self.lastMaterial error:nil]);
        }
    }
    checkCorrectness:^{
        XCTAssertTrue([self.fileManager fileExistsAtPath:self.firstMaterial]);
    }];
}

- (void)test_repair
{
    [self
    measure:^{
        TestCaseAssertTrue([self.database retrieve:nil] == 1.0f);
    }
    setUp:^{
        TestCaseAssertTrue([self fillDatabase:self.configForSizeToRepair]);
        TestCaseAssertTrue([self.database backup]);
    }
    tearDown:^{
    }
    checkCorrectness:^{
    }];
}

- (void)test_repair_without_backup
{
    [self
    measure:^{
        TestCaseAssertTrue([self.database retrieve:nil] == 1.0f);
    }
    setUp:^{
        TestCaseAssertTrue([self fillDatabase:self.configForSizeToRepair]);
    }
    tearDown:^{
    }
    checkCorrectness:^{
    }];
}

@end
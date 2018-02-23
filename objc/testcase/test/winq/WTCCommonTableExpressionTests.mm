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

#import "WTCWINQTestCase.h"

@interface WTCCommonTableExpressionTests : WTCWINQTestCase

@end

@implementation WTCCommonTableExpressionTests

- (void)testCommonTableExpression
{
    WINQAssertEqual(WCDB::CommonTableExpression(self.class.tableName)
                        .byAddingColumn(self.class.column)
                        .as(self.class.statementSelect),
                    @"testTable(testColumn) AS(SELECT testColumn FROM testTable)");

    WINQAssertEqual(WCDB::CommonTableExpression(self.class.tableName)
                        .byAddingColumns(self.class.columns)
                        .as(self.class.statementSelect),
                    @"testTable(testColumn, testColumn2) AS(SELECT testColumn FROM testTable)");

    WINQAssertEqual(WCDB::CommonTableExpression(self.class.tableName)
                        .as(self.class.statementSelect),
                    @"testTable AS(SELECT testColumn FROM testTable)");
}

@end
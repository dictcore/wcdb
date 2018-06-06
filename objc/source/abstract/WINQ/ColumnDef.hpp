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

#ifndef ColumnDef_hpp
#define ColumnDef_hpp

#include <WCDB/Describable.hpp>

namespace WCDB {

class ColumnDef : public DescribableWithLang<Lang::ColumnDef> {
public:
    ColumnDef(const Column &column);

    ColumnDef &withType(const ColumnType &columnType);
    ColumnDef &byAddingConstraint(const ColumnConstraint &columnConstraint);
    ColumnDef &
    byAddingConstraints(const std::list<ColumnConstraint> &columnConstraints);

    bool isAutoIncrement() const;
    bool isPrimary() const;
    const std::string &getColumnName() const;
};

} // namespace WCDB

#endif /* ColumnDef_hpp */
/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef COMMON_BASE_DATA_H
#define COMMON_BASE_DATA_H
#include <functional>
#include "serialize.h"

namespace oeaware {
class BaseData {
public:
    virtual void Serialize(oeaware::OutStream &out) const = 0;
    virtual void Deserialize(oeaware::InStream &in) = 0;
    static void RegisterClass(const std::string &key, std::function<std::shared_ptr<BaseData>()> constructor);
    static void RegisterClass(const std::vector<std::string> &keys,
        std::function<std::shared_ptr<BaseData>()> constructor);
    static std::shared_ptr<BaseData> Create(const std::string &type);
};
}

#endif

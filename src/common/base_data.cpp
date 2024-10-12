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
#include "base_data.h"
#include <unordered_map>

namespace oeaware {
static std::unordered_map<std::string, std::function<std::shared_ptr<BaseData>()>>& GetRegistry()
{
    static std::unordered_map<std::string, std::function<std::shared_ptr<BaseData>()>> registry;
    return registry;
}

void BaseData::RegisterClass(const std::string &key, std::function<std::shared_ptr<BaseData>()> constructor)
{
    GetRegistry()[key] = constructor;
}

void BaseData::RegisterClass(const std::vector<std::string> &keys,
    std::function<std::shared_ptr<BaseData>()> constructor)
{
    for (auto &key : keys) {
        GetRegistry()[key] = constructor;
    }
}

std::shared_ptr<BaseData> BaseData::Create(const std::string &type)
{
    auto it = GetRegistry().find(type);
    if (it != GetRegistry().end()) {
        return it->second();
    }
    return nullptr;
}
}

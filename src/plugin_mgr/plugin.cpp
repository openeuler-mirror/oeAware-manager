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
#include "plugin.h"

namespace oeaware {
const std::string Instance::pluginEnabled = "running";
const std::string Instance::pluginDisabled = "close";
const std::string Instance::pluginStateOn = "available";
const std::string Instance::pluginStateOff = "unavailable";

int Plugin::Load(const std::string &dl_path)
{
    this->handler = dlopen(dl_path.c_str(), RTLD_LAZY);
    if (handler == nullptr) {
        return -1;
    }
    return 0;
}

std::string Instance::GetInfo() const
{
    std::string stateText = this->state ? pluginStateOn : pluginStateOff;
    std::string runText = this->enabled ? pluginEnabled : pluginDisabled;
    return name + "(" + stateText + ", " + runText + ")";
}

std::vector<std::string> Instance::GetDeps()
{
    std::vector<std::string> vec;
    if (GetInterface()->get_dep == nullptr || GetInterface()->get_dep() == nullptr) {
        return vec;
    }
    std::string deps = GetInterface()->get_dep();
    std::string dep = "";
    for (size_t i = 0; i < deps.length(); ++i) {
        if (deps[i] != '-') {
            dep += deps[i];
        } else {
            vec.emplace_back(dep);
            dep = "";
        }
    }
    if (!dep.empty()) {
        vec.emplace_back(dep);
    }
    return vec;
}
}

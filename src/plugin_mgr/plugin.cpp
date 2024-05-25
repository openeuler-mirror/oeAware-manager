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

const std::string Instance::PLUGIN_ENABLED = "running";
const std::string Instance::PLUGIN_DISABLED = "close";
const std::string Instance::PLUGIN_STATE_ON = "available";
const std::string Instance::PLUGIN_STATE_OFF = "unavailable";

int Plugin::load(const std::string &dl_path) {
    void *handler = dlopen(dl_path.c_str(), RTLD_LAZY);
    if (handler == nullptr) {
        return -1;
    }
    this->handler = handler;
    return 0;
}

std::string Instance::get_info() const {
    std::string state_text = this->state ? PLUGIN_STATE_ON : PLUGIN_STATE_OFF;
    std::string run_text = this->enabled ? PLUGIN_ENABLED : PLUGIN_DISABLED;
    return name + "(" + state_text + ", " + run_text + ")"; 
}

std::vector<std::string> Instance::get_deps() {
    std::vector<std::string> vec;
    if (get_interface()->get_dep == nullptr) {
        return vec;
    }
    std::string deps = get_interface()->get_dep();
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

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

int Plugin::load(const std::string dl_path) {
    void *handler = dlopen(dl_path.c_str(), RTLD_LAZY);
    if (handler == nullptr) {
        printf("dlopen error!\n");
        return -1;
    }
    this->handler = handler;
    return 0;
}

std::string plugin_type_to_string(PluginType type) {
    switch (type) {
        case PluginType::COLLECTOR: {
            return "collector";
        }
        case PluginType::SCENARIO: {
            return "scenario";
        }
        case PluginType::TUNE: {
            return "tune";
        }
    }
    return "error";
}
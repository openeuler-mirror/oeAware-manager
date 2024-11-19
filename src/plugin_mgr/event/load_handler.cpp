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
#include "load_handler.h"
#include <sys/stat.h>
#include "utils.h"
#include "default_path.h"

namespace oeaware {
ErrorCode LoadHandler::LoadPlugin(const std::string &name)
{
    std::string plugin_path = DEFAULT_PLUGIN_PATH + "/" + name;
    if (!FileExist(plugin_path)) {
        return ErrorCode::LOAD_PLUGIN_FILE_NOT_EXIST;
    }
    if (!EndWith(name, ".so")) {
        return ErrorCode::LOAD_PLUGIN_FILE_IS_NOT_SO;
    }
    if (!CheckPermission(plugin_path, S_IRUSR | S_IRGRP)) {
        return ErrorCode::LOAD_PLUGIN_FILE_PERMISSION_DEFINED;
    }
    if (memoryStore->IsPluginExist(name)) {
        return ErrorCode::LOAD_PLUGIN_EXIST;
    }
    auto plugin = std::make_shared<Plugin>(name);
    plugin->recvQueue = recvQueue;
    int error = plugin->Load(plugin_path);
    if (error) {
        WARN(logger, dlerror());
        return ErrorCode::LOAD_PLUGIN_DLOPEN_FAILED;
    }
    memoryStore->AddPlugin(name, plugin);
    for (size_t i = 0; i < plugin->GetInstanceLen(); ++i) {
        memoryStore->AddInstance(plugin->GetInstance(i));
    }
    return ErrorCode::OK;
}

EventResult LoadHandler::Handle(const Event &event)
{
    std::string name = event.payload[0];
    auto retCode = LoadPlugin(name);
    EventResult eventResult;
    if (retCode == ErrorCode::OK) {
        INFO(logger, name << " plugin loaded.");
        eventResult.opt = Opt::RESPONSE_OK;
    } else {
        WARN(logger, name << " " << ErrorText::GetErrorText(retCode)  << ".");
        eventResult.opt = Opt::RESPONSE_ERROR;
        eventResult.payload.emplace_back(ErrorText::GetErrorText(retCode));
    }
    return eventResult;
}
}

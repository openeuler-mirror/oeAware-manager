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
#include "remove_handler.h"

namespace oeaware {
ErrorCode RemoveHandler::Remove(const std::string &name)
{
    if (!memoryStore->IsPluginExist(name)) {
        WARN("[PluginManager] " << name << " " << ErrorText::GetErrorText(ErrorCode::REMOVE_PLUGIN_NOT_EXIST) + ".");
        return ErrorCode::REMOVE_PLUGIN_NOT_EXIST;
    }
    std::shared_ptr<Plugin> plugin = memoryStore->GetPlugin(name);
    std::vector<std::string> instanceNames;
    for (size_t i = 0; i < plugin->GetInstanceLen(); ++i) {
        std::shared_ptr<Instance> instance = plugin->GetInstance(i);
        std::string iname = instance->GetName();
        if (instance->GetEnabled()) {
            WARN("[PluginManager] " << iname << " " <<
                ErrorText::GetErrorText(ErrorCode::REMOVE_INSTANCE_IS_RUNNING) << ".");
            return ErrorCode::REMOVE_INSTANCE_IS_RUNNING;
        }
        if (memoryStore->HaveDep(iname)) {
            WARN("[PluginManager] " << iname << " " << ErrorText::GetErrorText(ErrorCode::REMOVE_INSTANCE_HAVE_DEP) <<
                ".");
            return ErrorCode::REMOVE_INSTANCE_HAVE_DEP;
        }
        instanceNames.emplace_back(iname);
    }
    for (auto &iname : instanceNames) {
        memoryStore->DeleteInstance(iname);
    }
    memoryStore->DeletePlugin(name);
    return ErrorCode::OK;
}

EventResult RemoveHandler::Handle(const Event &event)
{
    std::string name = event.GetPayload(0);
    auto retCode = Remove(name);
    EventResult eventResult;
    if (retCode == ErrorCode::OK) {
        INFO("[PluginManager] " << name << " plugin removed.");
        eventResult.SetOpt(Opt::RESPONSE_OK);
    } else {
        eventResult.SetOpt(Opt::RESPONSE_ERROR);
        eventResult.AddPayload(ErrorText::GetErrorText(retCode));
    }
    return eventResult;
}
}
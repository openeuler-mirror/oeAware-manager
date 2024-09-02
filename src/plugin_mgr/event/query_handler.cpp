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
#include "query_handler.h"

namespace oeaware {
ErrorCode QueryHandler::QueryAllPlugins(std::string &res)
{
    std::vector<std::shared_ptr<Plugin>> allPlugins = memoryStore->GetAllPlugins();
    for (auto &p : allPlugins) {
        res += p->GetName() + "\n";
        for (size_t i = 0; i < p->GetInstanceLen(); ++i) {
            std::string info = p->GetInstance(i)->GetInfo();
            res += "\t" + info + "\n";
        }
    }
    return ErrorCode::OK;
}

ErrorCode QueryHandler::QueryPlugin(const std::string &name, std::string &res)
{
    if (!memoryStore->IsPluginExist(name)) {
        return ErrorCode::QUERY_PLUGIN_NOT_EXIST;
    }
    std::shared_ptr<Plugin> plugin = memoryStore->GetPlugin(name);
    res += name + "\n";
    for (size_t i = 0; i < plugin->GetInstanceLen(); ++i) {
        std::string info = plugin->GetInstance(i)->GetInfo();
        res += "\t" + info + "\n";
    }
    return ErrorCode::OK;
}

EventResult QueryHandler::Handle(const Event &event)
{
    EventResult eventResult;
    std::string resText;
    if (event.GetOpt() == Opt::QUERY_ALL) {
        INFO("[PluginManager] query all plugins information.");
        QueryAllPlugins(resText);
        eventResult.SetOpt(Opt::RESPONSE_OK);
        eventResult.AddPayload(resText);
    } else {
        auto name = event.GetPayload(0);
        auto retCode = QueryPlugin(name, resText);
        if (retCode == ErrorCode::OK) {
            INFO("[PluginManager] " << name << " plugin query successfully.");
            eventResult.SetOpt(Opt::RESPONSE_OK);
            eventResult.AddPayload(resText);
        } else {
            WARN("[PluginManager] " << name << " " << ErrorText::GetErrorText(retCode) + ".");
            eventResult.SetOpt(Opt::RESPONSE_ERROR);
            eventResult.AddPayload(ErrorText::GetErrorText(retCode));
        }
    }
    return eventResult;
}
}

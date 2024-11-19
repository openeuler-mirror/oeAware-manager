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
#include "query_subscribe_graph.h"

namespace oeaware {
std::string QuerySubscribeGraphHandler::GenerateDot(const std::vector<std::pair<std::string, std::string>> &graph)
{
    std::string res;
    res += "digraph G {\n";
    res += "    rankdir = TB\n";
    res += "    ranksep = 1\n";
    for (auto &p : graph) {
        res += "    " + p.first + "->\""  + p.second + "\";\n";
    }
    res += "}";
    return res;
}

ErrorCode QuerySubscribeGraphHandler::QuerySubGraph(const std::string &name,
    std::vector<std::pair<std::string, std::string>> &graph)
{
    if (!memoryStore->IsInstanceExist(name)) {
         return ErrorCode::QUERY_DEP_NOT_EXIST;
    }
    auto subscribers = instanceRunHandler->GetSubscribers();
    for (auto &p : subscribers) {
        if (!p.second.count(name)) {
            continue;
        }
        graph.emplace_back(std::make_pair(name, p.first));
    }
    return ErrorCode::OK;
}

EventResult QuerySubscribeGraphHandler::Handle(const Event &event)
{
    EventResult eventResult;
    std::string resText;
    std::vector<std::pair<std::string, std::string>> graph;
    if (event.opt == Opt::QUERY_ALL_SUB_GRAPH) {
        INFO(logger, "query all instance subscirbe topics.");
        auto plugins = memoryStore->GetAllPlugins();
        for (auto &plugin : plugins) {
            for (size_t i = 0; i < plugin->GetInstanceLen(); ++i) {
                auto instance = plugin->GetInstance(i);
                QuerySubGraph(instance->name, graph);
            }
        }
        resText = GenerateDot(graph);
        eventResult.opt = Opt::RESPONSE_OK;
        eventResult.payload.emplace_back(resText);
    } else {
        auto name = event.payload[0];
        auto retCode = QuerySubGraph(name, graph);
        resText = GenerateDot(graph);
        if (retCode == ErrorCode::OK) {
            INFO(logger, "query " << name << "instance subscirbe topics successfully.");
            eventResult.opt = Opt::RESPONSE_OK;
            eventResult.payload.emplace_back(resText);
        } else {
            WARN(logger, name << " " << ErrorText::GetErrorText(retCode) + ".");
            eventResult.opt = Opt::RESPONSE_ERROR;
            eventResult.payload.emplace_back(ErrorText::GetErrorText(retCode));
        }
    }
    return eventResult;
}
} // namespace oeaware

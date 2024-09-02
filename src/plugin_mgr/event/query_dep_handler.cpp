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
#include "query_dep_handler.h"

namespace oeaware {
std::string QueryDepHandler::GenerateDot(const std::vector<std::vector<std::string>> &query)
{
    std::string res;
    res += "digraph G {\n";
    res += "    rankdir = TB\n";
    res += "    ranksep = 1\n";
    std::unordered_map<std::string, std::unordered_set<std::string>> subGraph;
    for (auto &vec : query) {
        std::shared_ptr<Instance> instance = memoryStore->GetInstance(vec[0]);
        subGraph[instance->GetPluginName()].insert(vec[0]);
        if (vec.size() == 1) {
            continue;
        }
        if (memoryStore->IsInstanceExist(vec[1])) {
            instance = memoryStore->GetInstance(vec[1]);
            subGraph[instance->GetPluginName()].insert(vec[1]);
        } else {
            res += "    " + vec[1] + "[label=\"(missing)\\n" + vec[1] + "\", fontcolor=red];\n";
        }
        res += "    " + vec[0] + "->"  + vec[1] + ";\n";
    }
    int id = 0;
    for (auto &p : subGraph) {
        res += "    subgraph cluster_" + std::to_string(id) + " {\n";
        res += "        node [style=filled];\n";
        res += "        label = \"" + p.first + "\";\n";
        for (auto &i_name : p.second) {
            res += "        " + i_name + ";\n";
        }
        res += "    }\n";
        id++;
    }
    res += "}";
    return res;
}

ErrorCode QueryDepHandler::QueryDependency(const std::string &name, std::string &res)
{
    if (!memoryStore->IsInstanceExist(name)) {
        return ErrorCode::QUERY_DEP_NOT_EXIST;
    }
    DEBUG("[PluginManager] query top : " << name);
    std::vector<std::vector<std::string>> query;
    memoryStore->QueryNodeDependency(name, query);
    res = GenerateDot(query);
    return ErrorCode::OK;
}

ErrorCode QueryDepHandler::QueryAllDependencies(std::string &res)
{
    std::vector<std::vector<std::string>> query;
    memoryStore->QueryAllDependencies(query);
    DEBUG("[PluginManager] query size:" << query.size());
    res = GenerateDot(query);
    return ErrorCode::OK;
}

EventResult QueryDepHandler::Handle(const Event &event)
{
    EventResult eventResult;
    std::string resText;
    if (event.GetOpt() == Opt::QUERY_ALL_DEPS) {
        INFO("[PluginManager] query all instances dependencies.");
        QueryAllDependencies(resText);
        eventResult.SetOpt(Opt::RESPONSE_OK);
        eventResult.AddPayload(resText);
    } else {
        auto name = event.GetPayload(0);
        auto retCode = QueryDependency(name, resText);
        if (retCode == ErrorCode::OK) {
            INFO("[PluginManager] query " << name << " instance dependencies.");
            eventResult.SetOpt(Opt::RESPONSE_OK);
            eventResult.AddPayload(resText);
        } else {
            WARN("[PluginManager] query  " << name  << " instance dependencies failed, because " <<
                ErrorText::GetErrorText(retCode) << ".");
            eventResult.SetOpt(Opt::RESPONSE_ERROR);
            eventResult.AddPayload(ErrorText::GetErrorText(retCode));
        }
    }
    return eventResult;
}
}

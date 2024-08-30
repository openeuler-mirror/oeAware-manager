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
#ifndef PLUGIN_MGR_DEP_HANDLER_H
#define PLUGIN_MGR_DEP_HANDLER_H
#include <unordered_map>
#include <unordered_set>
#include "plugin.h"

namespace oeaware {
struct ArcNode {
    std::shared_ptr<ArcNode> next;
    std::string from;
    std::string to;
    bool isExist;  /* Whether this edge exists. */
    bool init;      /* The initial state of the edge. */
    ArcNode() { }
    ArcNode(const std::string &from, const std::string &to) : next(nullptr), from(from), to(to), isExist(false),
        init(false) { }
};

// a instance node
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<ArcNode> head;
    std::shared_ptr<Instance> instance;
    int cnt; /* Number of dependencies required for loading. */
    int realCnt; /* Actual number of dependencies during loading. */
    Node(): next(nullptr), head(nullptr), cnt(0), realCnt(0) { }
};

struct pair_hash {
    std::size_t operator() (const std::pair<std::string, std::string> &pair) const
    {
        auto h1 = std::hash<std::string>{}(pair.first);
        auto h2 = std::hash<std::string>{}(pair.second);
        return h1 ^ h2;
    }
};

class DepHandler {
public:
    DepHandler() {
        this->head = std::make_shared<Node>();
    }
    std::shared_ptr<Node> GetNode(const std::string &name);
    bool GetNodeState(const std::string &name)
    {
        return this->nodes[name]->instance->GetState();
    }
    void DeleteEdge(const std::string &from, const std::string &to);
    void AddEdge(const std::string &from, const std::string &to);
    void AddInstance(std::shared_ptr<Instance> instance);
    void DeleteInstance(const std::string &name);
    bool IsInstanceExist(const std::string &name);
    std::shared_ptr<Instance> GetInstance(const std::string &name) const
    {
        if (!nodes.count(name)) {
            return nullptr;
        }
        return nodes.at(name)->instance;
    }
    void QueryNodeDependency(const std::string &name, std::vector<std::vector<std::string>> &query);
    void QueryAllDependencies(std::vector<std::vector<std::string>> &query);
    /* check whether the instance has dependencies */
    bool HaveDep(const std::string &name)
    {
        return inEdges.count(name);
    }
private:
    void AddNode(std::shared_ptr<Instance> instance);
    void DelNode(const std::string &name);
    void QueryNodeTop(const std::string &name, std::vector<std::vector<std::string>> &query);
    void AddArcNode(std::shared_ptr<Node> node, const std::vector<std::string> &depNodes);
    void UpdateInstanceState(const std::string &name);
    void DelNodeAndArcNodes(std::shared_ptr<Node> node);
    std::shared_ptr<Node> AddNewNode(std::shared_ptr<Instance> instance);
    /* Indegree edges. */
    std::unordered_map<std::string, std::unordered_set<std::string>> inEdges;
    /* Store all edges. */
    std::unordered_map<std::pair<std::string, std::string>, std::shared_ptr<ArcNode>, pair_hash> arcNodes;
    /* Store all points. */
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::shared_ptr<Node> head;
};
}

#endif // !PLUGIN_MGR_DEP_HANDLER_H

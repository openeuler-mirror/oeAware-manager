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
#include "dep_handler.h"
#include <queue>

namespace oeaware {
bool DepHandler::IsInstanceExist(const std::string &name)
{
    return nodes.count(name);
}

void DepHandler::AddInstance(std::shared_ptr<Instance> instance)
{
    AddNode(instance);
}

void DepHandler::DeleteInstance(const std::string &name)
{
    DelNode(name);
}

void DepHandler::AddArcNode(std::shared_ptr<Node> node, const std::vector<std::string> &depNodes)
{
    std::shared_ptr<ArcNode> arcHead = node->head;
    node->cnt = depNodes.size();
    int realCnt = 0;
    bool state = true;
    for (auto name : depNodes) {
        std::string from = node->instance->GetName();
        std::shared_ptr<ArcNode> tmp = std::make_shared<ArcNode>(from, name);
        tmp->next = arcHead->next;
        arcHead->next = tmp;
        tmp->init = true;
        if (nodes.count(name)) {
            tmp->isExist = true;
            realCnt++;
            if (!nodes[name]->instance->GetState()) {
                state = false;
            }
        }
        inEdges[name].insert(from);
        arcNodes[std::make_pair(from, name)] = tmp;
    }
    /* node->instance->state = true, only all dependencies meet the conditions. */
    if (realCnt == node->cnt) {
        node->instance->SetState(state);
    }
    node->realCnt = realCnt;
}

void DepHandler::AddEdge(const std::string &from, const std::string &to)
{
    if (inEdges[to].find(from) != inEdges[to].end()) {
        auto arcNode = arcNodes[std::make_pair(from, to)];
        arcNode->isExist = true;
        return;
    }

    auto arcNode = std::make_shared<ArcNode>(from, to);
    auto node = nodes[from];
    arcNode->next = node->head->next;
    node->head->next = arcNode;
    arcNode->isExist = true;
    arcNodes[std::make_pair(from, to)] = arcNode;
    inEdges[to].insert(from);
}

void DepHandler::DeleteEdge(const std::string &from, const std::string &to)
{
    if (inEdges[to].find(from) == inEdges[to].end()) {
        return;
    }
    auto arcNode = arcNodes[std::make_pair(from, to)];
    arcNode->isExist = false;
}

void DepHandler::AddNode(std::shared_ptr<Instance> instance)
{
    std::string name = instance->GetName();
    std::shared_ptr<Node> curNode = AddNewNode(instance);
    this->nodes[name] = curNode;
    std::vector<std::string> depNodes = instance->GetDeps();
    AddArcNode(curNode, depNodes);
    UpdateInstanceState(name);
}

void DepHandler::DelNode(const std::string &name)
{
    DelNodeAndArcNodes(GetNode(name));
    this->nodes.erase(name);
}


std::shared_ptr<Node> DepHandler::GetNode(const std::string &name)
{
    return this->nodes[name];
}

std::shared_ptr<Node> DepHandler::AddNewNode(std::shared_ptr<Instance> instance)
{
    std::shared_ptr<Node> curNode = std::make_shared<Node>();
    curNode->instance = instance;
    curNode->head = std::make_shared<ArcNode>();
    return curNode;
}

void DepHandler::DelNodeAndArcNodes(std::shared_ptr<Node> node)
{
    std::shared_ptr<ArcNode> arc = node->head;
    while (arc) {
        std::shared_ptr<ArcNode> tmp = arc->next;
        if (arc != node->head) {
            std::string name = arc->to;
            node->realCnt--;
            inEdges[name].erase(arc->from);
            arcNodes.erase(std::make_pair(arc->from, arc->to));
            if (inEdges[name].empty()) {
                inEdges.erase(name);
            }
        }
        arc = tmp;
    }
}

void DepHandler::UpdateInstanceState(const std::string &name)
{
    if (!inEdges.count(name)) return;
    std::unordered_set<std::string> &arcs = inEdges[name];
    for (auto &from : arcs) {
        auto arcNode = arcNodes[std::make_pair(from, name)];
        if (nodes.count(from)) {
            auto tmp = nodes[from];
            if (!arcNode->isExist) {
                tmp->realCnt++;
            }
            if (tmp->realCnt == tmp->cnt && nodes[name]->instance->GetState()) {
                tmp->instance->SetState(true);
            }
            arcNode->isExist = true;
            UpdateInstanceState(tmp->instance->GetName());
        }
    }
}

void DepHandler::QueryAllDependencies(std::vector<std::vector<std::string>> &query)
{
    for (auto &p : nodes) {
        QueryNodeTop(p.first, query);
    }
}

void DepHandler::QueryNodeTop(const std::string &name, std::vector<std::vector<std::string>> &query)
{
    std::shared_ptr<ArcNode> arcNode = nodes[name]->head;
    if (arcNode->next == nullptr) {
        query.emplace_back(std::vector<std::string>{name});
        return;
    }
    while (arcNode->next != nullptr) {
        /* Display the edges that exist and the points that do not. */
        if (arcNode->next->isExist || !nodes.count(arcNode->next->to)) {
            query.emplace_back(std::vector<std::string>{name, arcNode->next->to});
        }
        arcNode = arcNode->next;
    }
}

void DepHandler::QueryNodeDependency(const std::string &name, std::vector<std::vector<std::string>> &query)
{
    if (!nodes.count(name)) return;
    std::queue<std::string> instanceQueue;
    std::unordered_set<std::string> vis;
    vis.insert(name);
    instanceQueue.push(name);
    while (!instanceQueue.empty()) {
        auto node = nodes[instanceQueue.front()];
        instanceQueue.pop();
        std::string nodeName = node->instance->GetName();
        query.emplace_back(std::vector<std::string>{nodeName});
        for (auto cur = node->head->next; cur != nullptr; cur = cur->next) {
            if (cur->isExist || !nodes.count(cur->to)) {
                query.emplace_back(std::vector<std::string>{nodeName, cur->to});
            }
            if (!vis.count(cur->to) && nodes.count(cur->to)) {
                vis.insert(cur->to);
                instanceQueue.push(cur->to);
            }
        }
    }
}
}

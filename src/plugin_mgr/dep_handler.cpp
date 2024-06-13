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
    
bool DepHandler::is_instance_exist(const std::string &name) {
    return nodes.count(name);
}   

void DepHandler::add_instance(std::shared_ptr<Instance> instance) {
    add_node(instance);
}

void DepHandler::delete_instance(const std::string &name) {
    del_node(name);
}

void DepHandler::add_arc_node(std::shared_ptr<Node> node, const std::vector<std::string> &dep_nodes) {
    std::shared_ptr<ArcNode> arc_head = node->head;
    node->cnt = dep_nodes.size();
    int real_cnt = 0;
    bool state = true;
    for (auto name : dep_nodes) {
        std::string from = node->instance->get_name();
        std::shared_ptr<ArcNode> tmp = std::make_shared<ArcNode>(from, name);
        tmp->next = arc_head->next;
        arc_head->next = tmp;
        tmp->init = true;
        if (nodes.count(name)) {
            tmp->is_exist = true;
            real_cnt++;
            if (!nodes[name]->instance->get_state()) {
                state = false;
            }
        }
        in_edges[name].insert(from);
        arc_nodes[std::make_pair(from, name)] = tmp;
    }
    /* node->instance->state = true, only all dependencies meet the conditions. */
    if (real_cnt == node->cnt) {
        node->instance->set_state(state);
    }
    node->real_cnt = real_cnt;
} 

void DepHandler::add_edge(const std::string &from, const std::string &to) {
    if (in_edges[to].find(from) != in_edges[to].end()) { 
        auto arc_node = arc_nodes[std::make_pair(from, to)];
        arc_node->is_exist = true;
        return;
    }
   
    auto arc_node = std::make_shared<ArcNode>(from, to);
    auto node = nodes[from];
    arc_node->next = node->head->next;
    node->head->next = arc_node;
    arc_node->is_exist = true;
    arc_nodes[std::make_pair(from, to)] = arc_node;
    in_edges[to].insert(from);
}

void DepHandler::delete_edge(const std::string &from, const std::string &to) {
    if (in_edges[to].find(from) == in_edges[to].end()) {
        return;
    }
    auto arc_node = arc_nodes[std::make_pair(from, to)]; 
    arc_node->is_exist = false;
}

void DepHandler::add_node(std::shared_ptr<Instance> instance) {
    std::string name = instance->get_name();
    std::shared_ptr<Node> cur_node = add_new_node(instance);
    this->nodes[name] = cur_node;
    std::vector<std::string> dep_nodes = instance->get_deps();
    add_arc_node(cur_node, dep_nodes);
    update_instance_state(name);
}

void DepHandler::del_node(const std::string &name) {
    del_node_and_arc_nodes(get_node(name));
    this->nodes.erase(name);
}


std::shared_ptr<Node> DepHandler::get_node(const std::string &name) {
    return this->nodes[name];
}

std::shared_ptr<Node> DepHandler::add_new_node(std::shared_ptr<Instance> instance) {
    std::shared_ptr<Node> cur_node = std::make_shared<Node>();
    cur_node->instance = instance;
    cur_node->head = std::make_shared<ArcNode>();
    return cur_node;
}

void DepHandler::del_node_and_arc_nodes(std::shared_ptr<Node> node) {
    std::shared_ptr<ArcNode> arc = node->head;
    while(arc) {
        std::shared_ptr<ArcNode> tmp = arc->next;
        if (arc != node->head){
            std::string name = arc->to;
            in_edges[name].erase(arc->from);
            arc_nodes.erase(std::make_pair(arc->from, arc->to));
            if (in_edges[name].empty()) {
                in_edges.erase(name);
            }
        }
        arc = tmp; 
    } 
}

void DepHandler::update_instance_state(const std::string &name) {
    if (!in_edges.count(name)) return;
    std::unordered_set<std::string> &arcs = in_edges[name];
    for (auto &from : arcs) {
        auto arc_node = arc_nodes[std::make_pair(from, name)];
        if (nodes.count(from)) {
            auto tmp = nodes[from];
            tmp->real_cnt++;
            if (tmp->real_cnt == tmp->cnt && nodes[name]->instance->get_state()) {
                tmp->instance->set_state(true);
            }
            arc_node->is_exist = true;
            update_instance_state(tmp->instance->get_name());
        }
    }
}

void DepHandler::query_all_dependencies(std::vector<std::vector<std::string>> &query) {
    for (auto &p : nodes) {
        query_node_top(p.first, query);
    }
}

void DepHandler::query_node_top(const std::string &name, std::vector<std::vector<std::string>> &query) {
    std::shared_ptr<ArcNode> arc_node = nodes[name]->head;
    if (arc_node->next == nullptr) {
        query.emplace_back(std::vector<std::string>{name});
        return;
    }
    while (arc_node->next != nullptr) { 
        /* Display the edges that exist and the points that do not. */      
        if (arc_node->next->is_exist || !nodes.count(arc_node->next->to)) {
            query.emplace_back(std::vector<std::string>{name, arc_node->next->to});
        }
        arc_node = arc_node->next;
    }
}

void DepHandler::query_node_dependency(const std::string &name, std::vector<std::vector<std::string>> &query) {
    if (!nodes.count(name)) return;
    std::queue<std::string> instance_queue;
    std::unordered_set<std::string> vis;
    vis.insert(name);
    instance_queue.push(name);
    while (!instance_queue.empty()) {
        auto node = nodes[instance_queue.front()];
        instance_queue.pop();
        std::string node_name = node->instance->get_name();
        query.emplace_back(std::vector<std::string>{node_name});
        for (auto cur = node->head->next; cur != nullptr; cur = cur->next) {
            if (cur->is_exist || !nodes.count(cur->to)) {
                query.emplace_back(std::vector<std::string>{node_name, cur->to});
            }
            if (!vis.count(cur->to) && nodes.count(cur->to)) {
                vis.insert(cur->to);
                instance_queue.push(cur->to);
            }
        }
    }
}

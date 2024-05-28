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
    for (auto name : dep_nodes) {
        std::shared_ptr<ArcNode> tmp = std::make_shared<ArcNode>();
        tmp->arc_name = name; 
        tmp->node_name = node->instance->get_name();
        tmp->next = arc_head->next;
        arc_head->next = tmp;
        
        if (nodes.count(name)) {
            tmp->is_exist = true;
            arc_nodes[name].insert(tmp);
            real_cnt++;
        } else {
            tmp->is_exist = false;
            arc_nodes[name].insert(tmp);
        }
    }
    if (real_cnt == node->cnt) {
        node->instance->set_state(true);
    }
    node->real_cnt = real_cnt;
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
            std::string name = arc->arc_name;
            arc_nodes[name].erase(arc);
            if (arc_nodes[name].empty()) {
                arc_nodes.erase(name);
            }
        }
        arc = tmp; 
    } 
}

void DepHandler::update_instance_state(const std::string &name) {
    if (!nodes[name]->instance->get_state() || !arc_nodes.count(name)) return;
    std::unordered_set<std::shared_ptr<ArcNode>> &arcs = arc_nodes[name];
    for (auto &arc_node : arcs) {
        if (nodes.count(arc_node->node_name)) {
            auto tmp = nodes[arc_node->node_name];
            tmp->real_cnt++;
            if (tmp->real_cnt == tmp->cnt) {
                tmp->instance->set_state(true);
            }
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
    std::shared_ptr<ArcNode> p = nodes[name]->head;
    if (p->next == nullptr) {
        query.emplace_back(std::vector<std::string>{name});
        return;
    }
    while (p->next != nullptr) {       
        query.emplace_back(std::vector<std::string>{name, p->next->arc_name});
        p = p->next;
    }
}

void DepHandler::query_node_dependency(const std::string &name, std::vector<std::vector<std::string>> &query) {
    if (!nodes.count(name)) return;
    std::queue<std::string> q;
    std::unordered_set<std::string> vis;
    vis.insert(name);
    q.push(name);
    while (!q.empty()) {
        auto node = nodes[q.front()];
        q.pop();
        std::string node_name = node->instance->get_name();
        query.emplace_back(std::vector<std::string>{node_name});
        for (auto cur = node->head->next; cur != nullptr; cur = cur->next) {
            query.emplace_back(std::vector<std::string>{node_name, cur->arc_name});
            if (!vis.count(cur->arc_name) && nodes.count(cur->arc_name)) {
                vis.insert(cur->arc_name);
                q.push(cur->arc_name);
            }
        }
    }
}

std::vector<std::string> DepHandler::get_pre_dependencies(const std::string &name) {
    std::vector<std::string> res;
    std::queue<std::shared_ptr<Node>> q;
    std::unordered_set<std::string> vis;
    vis.insert(name);
    q.push(nodes[name]);
    while (!q.empty()) {
        auto &node = q.front();
        q.pop();
        res.emplace_back(node->instance->get_name());
        for (auto arc_node = node->head->next; arc_node != nullptr; arc_node = arc_node->next) {
            if (!vis.count(arc_node->arc_name)) {
                vis.insert(arc_node->arc_name);
                q.push(nodes[arc_node->arc_name]);
            }
        }
    }
    return res;
}

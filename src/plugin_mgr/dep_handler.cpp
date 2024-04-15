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
#include <stdio.h>
    
void DepHandler::add_arc_node(Node* node, const std::vector<std::string> &dep_nodes) {
    ArcNode *arc_head = new ArcNode();
    node->head = arc_head;
    node->cnt = dep_nodes.size();
    int real_cnt = 0;
    bool state = true;
    for (auto val : dep_nodes) {
        ArcNode *tmp = new ArcNode();
        tmp->val = val; 
        tmp->node_name = node->val;
        tmp->next = arc_head->next; 
        if (arc_head->next != nullptr) {
            arc_head->next->pre = tmp;
        }
        
        tmp->pre = arc_head;
        arc_head->next = tmp;
        
        if (nodes.count(val)) {
            arc_nodes[val][tmp] = true;
            real_cnt++;
        } else {
            arc_nodes[val][tmp] = false;
            state = false;
        }
    }
    node->state = state;
    node->real_cnt = real_cnt;
}


void DepHandler::add_node(std::string name, std::vector<std::string> dep_nodes) {
    Node *cur_node = add_new_node(name);
    change_arc_nodes(name, true);
    add_arc_node(cur_node, dep_nodes);
    this->nodes[name] = cur_node;
}

void DepHandler::del_node(std::string name) {
    del_node_and_arc_nodes(get_node(name));
    this->nodes.erase(name);
    change_arc_nodes(name, false);
}


Node* DepHandler::get_node(std::string name) {
    return this->nodes[name];
}


Node* DepHandler::add_new_node(std::string name) {
    Node *cur_node = new Node(name);
    tail->next = cur_node;
    cur_node->pre = tail;
    tail = cur_node;
    return cur_node;
}



void DepHandler::del_node_and_arc_nodes(Node *node) {
    Node *pre = node->pre;
    Node *next = node->next;
    pre->next = next;
    if (next != nullptr)
        next->pre = pre;
    ArcNode *arc = node->head;
    while(arc) {
        ArcNode *tmp = arc->next;
        if (arc != node->head){
            arc_nodes[node->val].erase(arc);
            if (arc_nodes[node->val].empty()) {
                arc_nodes.erase(node->val);
            }
        }
        delete arc;
        arc = tmp;
        
    } 
    delete node;
}
void DepHandler::change_arc_nodes(std::string name, bool state) {
    std::unordered_map<ArcNode*, bool> &mp = arc_nodes[name];
    for (auto &vec : mp) {
        vec.second = state;
        if (nodes.count(vec.first->node_name)) {
            Node *tmp = nodes[vec.first->node_name];
            if (state) {
                tmp->real_cnt++;
                if (tmp->real_cnt == tmp->cnt) {
                    tmp->state = true;
                }
            } else {
                tmp->real_cnt--;
                tmp->state = false;
            }
        }
    }
}


void DepHandler::query_all_top(std::vector<std::vector<std::string>> &query) {
    for (auto &p : nodes) {
        query_node_top(p.first, query);
    }
}

void DepHandler::query_node_top(std::string name, std::vector<std::vector<std::string>> &query) {
    ArcNode *p = nodes[name]->head;
    if (p->next == nullptr) {
        query.emplace_back(std::vector<std::string>{name});
        return;
    }
    while (p->next != nullptr) {       
        query.emplace_back(std::vector<std::string>{name, p->next->val});
        p = p->next;
    }
}

void DepHandler::query_node(std::string name, std::vector<std::vector<std::string>> &query) {
    if (!nodes.count(name)) return;
    Node *p = nodes[name];
    query.emplace_back(std::vector<std::string>{name});
    for (auto cur = p->head->next; cur != nullptr; cur = cur->next) {
        query.emplace_back(std::vector<std::string>{name, cur->val});
        query_node(cur->val, query);
    }
}

std::vector<std::string> DepHandler::get_pre_dependencies(std::string name) {
    std::vector<std::string> res;
    std::queue<Node*> q;
    q.push(nodes[name]);
    while (!q.empty()) {
        auto &node = q.front();
        q.pop();
        res.emplace_back(node->val);
        for (auto arc_node = node->head->next; arc_node != nullptr; arc_node = arc_node->next) {
            q.push(nodes[arc_node->val]);
        }
    }
    return res;
}

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
#include <string>
#include <vector>
#include <cstdint>

struct ArcNode {
    ArcNode *next;
    std::string arc_name;
    std::string node_name;
    ArcNode() : next(nullptr) {}
};

// a instance node 
struct Node {
    Node *next;
    ArcNode *head;
    std::string name;
    int cnt; 
    int real_cnt; 
    bool state; // dependency closed-loop
    Node() : next(nullptr), head(nullptr), state(true), cnt(0), real_cnt(0) {}
    Node(std::string name): name(name), next(nullptr), head(nullptr), state(true), cnt(0), real_cnt(0) {}
};

class DepHandler {
public:
    DepHandler() {
        this->head = new Node();
        this->tail = head;
    }
    Node* get_node(std::string name);
    bool get_node_state(std::string name) {
        return this->nodes[name]->state;
    }
    void add_node(std::string name, std::vector<std::string> dep_nodes = {});
    void del_node(std::string name);
    std::vector<std::string> get_pre_dependencies(std::string name);
    // query instance dependency
    void query_node(std::string name, std::vector<std::vector<std::string>> &query);
    // query all instance dependencies
    void query_all_top(std::vector<std::vector<std::string>> &query);
    bool have_dep(const std::string &name) {
        return arc_nodes.count(name);
    }
    bool is_empty() const {
        return nodes.empty();
    }
    size_t get_node_nums() const {
        return nodes.size();
    }
private:
    void query_node_top(std::string name, std::vector<std::vector<std::string>> &query);
    void add_arc_node(Node* node, const std::vector<std::string> &dep_nodes);
    void change_arc_nodes(std::string name, bool state);
    void del_node_and_arc_nodes(Node *node);
    Node* add_new_node(std::string name);

    std::unordered_map<std::string, std::unordered_map<ArcNode*, bool>> arc_nodes;
    std::unordered_map<std::string, Node*> nodes;
    Node * head;
    Node *tail;
};

#endif // !PLUGIN_MGR_DEP_HANDLER_H

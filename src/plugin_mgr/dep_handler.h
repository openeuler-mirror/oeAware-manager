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
#include <memory>

struct ArcNode {
    std::shared_ptr<ArcNode> next;
    std::string arc_name;
    std::string node_name;
    ArcNode() : next(nullptr) {}
};

// a instance node 
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<ArcNode> head;
    std::string name;
    int cnt; 
    int real_cnt; 
    bool state; // dependency closed-loop
    Node() : next(nullptr), head(nullptr), cnt(0), real_cnt(0), state(true) {}
    Node(std::string name): next(nullptr), head(nullptr), name(name), cnt(0), real_cnt(0), state(true) {}
};

class DepHandler {
public:
    DepHandler() {
        this->head = std::make_shared<Node>();
        this->tail = head;
    }
    std::shared_ptr<Node> get_node(std::string name);
    bool get_node_state(std::string name) {
        return this->nodes[name]->state;
    }
    void add_node(const std::string &name, std::vector<std::string> dep_nodes = {});
    void del_node(const std::string &name);
    std::vector<std::string> get_pre_dependencies(const std::string &name);
    // query instance dependency
    void query_node(const std::string &name, std::vector<std::vector<std::string>> &query);
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
    void add_arc_node(std::shared_ptr<Node> node, const std::vector<std::string> &dep_nodes);
    void change_arc_nodes(std::string name, bool state);
    void del_node_and_arc_nodes(std::shared_ptr<Node> node);
    std::shared_ptr<Node> add_new_node(std::string name);

    std::unordered_map<std::string, std::unordered_map<std::shared_ptr<ArcNode>, bool>> arc_nodes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::shared_ptr<Node> head;
    std::shared_ptr<Node> tail;
};

#endif // !PLUGIN_MGR_DEP_HANDLER_H

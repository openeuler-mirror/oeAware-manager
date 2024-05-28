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

#include "plugin.h"
#include <unordered_map>
#include <unordered_set>

struct ArcNode {
    std::shared_ptr<ArcNode> next;
    std::string arc_name;
    std::string node_name;
    bool is_exist;  
    ArcNode() : next(nullptr), is_exist(false) { }
};

// a instance node 
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<ArcNode> head;
    std::shared_ptr<Instance> instance;
    int cnt; 
    int real_cnt; 
    Node(): next(nullptr), head(nullptr), cnt(0), real_cnt(0) { }
};

class DepHandler {
public:
    DepHandler() {
        this->head = std::make_shared<Node>();
    }
    std::shared_ptr<Node> get_node(const std::string &name);
    bool get_node_state(const std::string &name) {
        return this->nodes[name]->instance->get_state();
    }
    void add_instance(std::shared_ptr<Instance> instance);
    void delete_instance(const std::string &name);
    bool is_instance_exist(const std::string &name);
    std::shared_ptr<Instance> get_instance(const std::string &name) const {
        return nodes.at(name)->instance;
    }
    void query_node_dependency(const std::string &name, std::vector<std::vector<std::string>> &query);
    void query_all_dependencies(std::vector<std::vector<std::string>> &query);
    /* check whether the instance has dependencies */
    bool have_dep(const std::string &name) {
        return arc_nodes.count(name);
    }
    std::vector<std::string> get_pre_dependencies(const std::string &name);
private:
    void add_node(std::shared_ptr<Instance> instance);
    void del_node(const std::string &name);
    void query_node_top(const std::string &name, std::vector<std::vector<std::string>> &query);
    void add_arc_node(std::shared_ptr<Node> node, const std::vector<std::string> &dep_nodes);
    void update_instance_state(const std::string &name);
    void del_node_and_arc_nodes(std::shared_ptr<Node> node);
    std::shared_ptr<Node> add_new_node(std::shared_ptr<Instance> instance);
    /* indegree edges */
    std::unordered_map<std::string, std::unordered_set<std::shared_ptr<ArcNode>>> arc_nodes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::shared_ptr<Node> head;
};

#endif // !PLUGIN_MGR_DEP_HANDLER_H

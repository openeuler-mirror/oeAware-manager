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
#ifndef PLUGIN_MGR_MEMORY_STORE_H
#define PLUGIN_MGR_MEMORY_STORE_H
#include "logger.h"
#include "dep_handler.h"
#include <unordered_map>

/* OeAware memory storage, which is used to store plugins and instances in the memory. */
class MemoryStore {
public:
    void add_plugin(const std::string &name, std::shared_ptr<Plugin> plugin) {
        this->plugins.insert(std::make_pair(name, plugin));
    }
    void add_instance(std::shared_ptr<Instance> instance) {
        dep_handler.add_instance(instance);
    }
    std::shared_ptr<Plugin> get_plugin(const std::string &name) const {
        return this->plugins.at(name);
    }
    std::shared_ptr<Instance> get_instance(const std::string &name) const {
        return dep_handler.get_instance(name);
    }
    void delete_plugin(const std::string &name) {
        this->plugins.erase(name);
    }
    void delete_instance(const std::string &name) {
        dep_handler.delete_instance(name);
    }
    bool is_plugin_exist(const std::string &name) const {
        return this->plugins.count(name);
    }
    bool is_instance_exist(const std::string &name) {
        return dep_handler.is_instance_exist(name);
    }
    const std::vector<std::shared_ptr<Plugin>> get_all_plugins() {
        std::vector<std::shared_ptr<Plugin>> res;
        for (auto &p : plugins) {
            res.emplace_back(p.second);
        }
        return res;
    }
    void query_node_dependency(const std::string &name, std::vector<std::vector<std::string>> &query) {
        return dep_handler.query_node_dependency(name, query);
    }
    void query_all_dependencies(std::vector<std::vector<std::string>> &query) {
        return dep_handler.query_all_dependencies(query);
    }
    bool have_dep(const std::string &name) {
        return dep_handler.have_dep(name);
    }
    std::vector<std::string> get_pre_dependencies(const std::string &name) {
        return dep_handler.get_pre_dependencies(name);
    }
private:
    /* instance are stored in the form of DAG. 
     * DepHandler stores instances and manages dependencies.
     */
    DepHandler dep_handler;
    std::unordered_map<std::string, std::shared_ptr<Plugin>> plugins; 
};

#endif // !PLUGIN_MGR_MEMORY_STORE_H

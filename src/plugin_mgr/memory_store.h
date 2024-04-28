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
#include "plugin.h"
#include "logger.h"
#include <unordered_map>
#include <memory>

//OeAware memory storage, which is used to store plugins and instances in the memory.
class MemoryStore {
public:
    void add_plugin(const std::string &name, std::shared_ptr<Plugin> plugin) {
        this->plugins.insert(std::make_pair(name, plugin));
    }
    void add_instance(const std::string &name, std::shared_ptr<Instance> instance) {
        this->instances.insert(std::make_pair(name, instance));
    }
    std::shared_ptr<Plugin> get_plugin(const std::string &name) const {
        return this->plugins.at(name);
    }
    std::shared_ptr<Instance> get_instance(const std::string &name) const {
        return this->instances.at(name);
    }
    void delete_plugin(const std::string &name) {
        this->plugins.erase(name);
    }
    void delete_instance(const std::string &name) {
        this->instances.erase(name);
    }
    bool is_plugin_exist(const std::string &name) const {
        return this->plugins.count(name);
    }
    bool is_instance_exist(const std::string &name) const {
        return this->instances.count(name);
    }
    std::vector<std::shared_ptr<Plugin>> get_all_plugins() {
        std::vector<std::shared_ptr<Plugin>> res;
        for (auto &p : plugins) {
            res.emplace_back(p.second);
        }
        return res;
    }
    std::vector<std::shared_ptr<Instance>> get_all_instances() {
        std::vector<std::shared_ptr<Instance>> res;
        for (auto &p : instances) {
            res.emplace_back(p.second);
        }
        return res;
    }
private:
    std::unordered_map<std::string, std::shared_ptr<Plugin>> plugins; 
    std::unordered_map<std::string, std::shared_ptr<Instance>> instances; 
};

#endif // !PLUGIN_MGR_MEMORY_STORE_H

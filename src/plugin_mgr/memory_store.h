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

//OeAware memory storage, which is used to store plugins and instances in the memory.
class MemoryStore {
public:
    void add_plugin(const std::string &name, Plugin *plugin) {
        this->plugins.insert(std::make_pair(name, plugin));
    }
    void add_instance(const std::string &name, Instance *instance) {
        this->instances.insert(std::make_pair(name, instance));
    }
    Plugin* get_plugin(const std::string &name) const {
        return this->plugins.at(name);
    }
    Instance* get_instance(const std::string &name) const {
        return this->instances.at(name);
    }
    void delete_plugin(const std::string &name) {
        Plugin *plugin = plugins.at(name);
        this->plugins.erase(name);
        delete plugin;
    }
    void delete_instance(const std::string &name) {
        Instance *instance = instances.at(name);
        this->instances.erase(name);
    }
    bool is_plugin_exist(const std::string &name) const {
        return this->plugins.count(name);
    }
    bool is_instance_exist(const std::string &name) const {
        return this->instances.count(name);
    }
    std::vector<Plugin*> get_all_plugins() {
        std::vector<Plugin*> res;
        for (auto &p : plugins) {
            res.emplace_back(p.second);
        }
        return res;
    }
    std::vector<Instance*> get_all_instances() {
        std::vector<Instance*> res;
        for (auto &p : instances) {
            res.emplace_back(p.second);
        }
        return res;
    }
private:
    std::unordered_map<std::string, Plugin*> plugins; 
    std::unordered_map<std::string, Instance*> instances; 
};

#endif // !PLUGIN_MGR_MEMORY_STORE_H

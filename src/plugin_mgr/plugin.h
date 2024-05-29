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
#ifndef PLUGIN_MGR_PLUGIN_H
#define PLUGIN_MGR_PLUGIN_H
#include "interface.h"
#include <string>
#include <vector>
#include <memory>
#include <dlfcn.h>

class Instance {
public:
    void set_name(const std::string &name)  {
        this->name = name;
    }
    std::string get_name() const {
        return this->name;
    }
    int get_priority() const {
        return interface->get_priority();
    }
    Interface* get_interface() const {
        return this->interface;
    }
    void set_plugin_name(const std::string &name)  {
        this->plugin_name = name;
    }
    std::string get_plugin_name() const {
        return this->plugin_name;
    }
    void set_state(bool state) {
        this->state = state;
    }
    bool get_state() const {
        return this->state;
    }
    void set_enabled(bool enabled) {
        this->enabled = enabled;
    }
    bool get_enabled() const {
        return this->enabled;
    }
    void set_interface(Interface *interface) {
        this->interface = interface;
    }
    std::string get_info() const;
    std::vector<std::string> get_deps();
private:
    std::string name;
    std::string plugin_name;
    bool state;
    bool enabled;
    Interface *interface;
    const static std::string PLUGIN_ENABLED;
    const static std::string PLUGIN_DISABLED;
    const static std::string PLUGIN_STATE_ON;
    const static std::string PLUGIN_STATE_OFF;
};

class Plugin {
public:
    explicit Plugin(const std::string &name) : name(name), handler(nullptr) { }
    ~Plugin() {   
        if (handler != nullptr) {
            dlclose(handler);
        }
    }
    int load(const std::string &dl_path);
    std::string get_name() const {
        return this->name;
    }
    void add_instance(std::shared_ptr<Instance> ins) {
        instances.emplace_back(ins);
    }
    std::shared_ptr<Instance> get_instance(size_t i) const {
        return instances[i];
    }
    size_t get_instance_len() const {
        return instances.size();
    }
    void* get_handler() const {
        return handler;
    }
private:
    std::vector<std::shared_ptr<Instance>> instances;
    std::string name;
    void *handler;
};

#endif 

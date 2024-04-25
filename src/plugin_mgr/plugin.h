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
#include <dlfcn.h>

enum class PluginType {
    COLLECTOR,
    SCENARIO,
    TUNE,
    EMPTY,
};

std::string plugin_type_to_string(PluginType type);

class Instance {
public:
    virtual ~Instance() = default;
    void set_name(std::string name)  {
        this->name = name;
    }
    std::string get_name() const {
        return this->name;
    }
    void set_type(PluginType type) {
        this->type = type;
    }
    PluginType get_type() const {
        return type;
    }
    void set_plugin_name(std::string name)  {
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
    std::string get_info() const;
private:
    std::string name;
    std::string plugin_name;
    PluginType type;
    bool state;
    bool enabled;
    const static std::string PLUGIN_ENABLED;
    const static std::string PLUGIN_DISABLED;
    const static std::string PLUGIN_STATE_ON;
    const static std::string PLUGIN_STATE_OFF;
};

class CollectorInstance : public Instance {
public:
    CollectorInstance() : interface(nullptr) { }
    ~CollectorInstance() {
        interface = nullptr;
    }
    void set_interface(CollectorInterface *interface) {
        this->interface = interface;
    }
    CollectorInterface* get_interface() const {
        return this->interface;
    }
private:
    CollectorInterface* interface;
};

class ScenarioInstance : public Instance {
public:
    ScenarioInstance() : interface(nullptr) { }
    ~ScenarioInstance() {
        interface = nullptr;
    }
    void set_interface(ScenarioInterface *interface) {
        this->interface = interface;
    }
    ScenarioInterface* get_interface() const {
        return this->interface;
    }
private:
    ScenarioInterface* interface;
};

class TuneInstance : public Instance {
public:
    TuneInstance() : interface(nullptr) { }
    ~TuneInstance() { 
        interface = nullptr;
    }
    void set_interface(TuneInterface *interface) {
        this->interface = interface;
    }
    TuneInterface* get_interface() const {
        return this->interface;
    }
private:
    TuneInterface *interface;
};

class Plugin {
public:
    Plugin(std::string name, PluginType type) : name(name), type(type), handler(nullptr) { }
    ~Plugin() {        
        for (int i = 0; i < instances.size(); ++i) {
            delete instances[i];
        }
        dlclose(this->handler);
    }
    int load(const std::string dl_path);
    std::string get_name() const {
        return this->name;
    }
    PluginType get_type() const {
        return this->type;
    }
    void add_instance(Instance *ins) {
        instances.emplace_back(ins);
    }
    Instance* get_instance(int i) const {
        return instances[i];
    }
    size_t get_instance_len() const {
        return instances.size();
    }
    void * get_handler() const {
        return handler;
    }
private:
    void *handler;
    std::vector<Instance*> instances;
    PluginType type;
    std::string name;
};

#endif 

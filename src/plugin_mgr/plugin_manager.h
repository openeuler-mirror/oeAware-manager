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
#ifndef PLUGIN_MGR_PLUGIN_MANAGER_H
#define PLUGIN_MGR_PLUGIN_MANAGER_H

#include "instance_run_handler.h"
#include "plugin.h"
#include "config.h"
#include "dep_handler.h"
#include "message_manager.h"
#include "logger.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <dlfcn.h>

class PluginManager {
public:
    PluginManager(SafeQueue<Message> *handler_msg, SafeQueue<Message> *res_msg) {
        this->handler_msg = handler_msg;
        this->res_msg = res_msg;
        dep_handler = new DepHandler();
        instance_run_handler = new InstanceRunHandler();
    }
    ~PluginManager() { }
    int run();
    void pre_load();
    void pre_enable();
    void init(Config *config);
    void* get_data_buffer(std::string name);
private:
    void pre_load_plugin(PluginType type); 
    bool check(char **deps, int len);
    bool query_all_plugins(Message &res);
    bool query_plugin(std::string name, Message &res);
    bool query_top(std::string name, Message &res);
    bool query_all_tops(Message &res);
    bool instance_enabled(std::string name);
    bool instance_disabled(std::string name);
    void instance_dep_check(std::string name, Message &res);
    template <typename T>
    int load_dl_instance(Plugin *plugin, T **interface_list);
    template <typename T, typename U>
    void save_instance(Plugin *plugin, T *interface_list, int len);
    bool load_instance(Plugin *plugin);
    bool load_plugin(const std::string path, PluginType type);
    void batch_load();
    bool remove(const std::string name);
    void batch_remove();
    void add_list(Message &msg);
    bool is_instance_exist(std::string name) {
        return this->instances.find(name) != this->instances.end();
    }
    void update_instance_state();
private:
    InstanceRunHandler *instance_run_handler;
    Config *config;
    SafeQueue<Message> *handler_msg;
    SafeQueue<Message> *res_msg;
    std::unordered_map<std::string, Plugin*> plugins; 
    std::unordered_map<std::string, Instance*> instances; 
    DepHandler *dep_handler; 
    std::unordered_map<std::string, PluginType> plugin_types;  
};

bool check_permission(std::string path, int mode);

#endif 
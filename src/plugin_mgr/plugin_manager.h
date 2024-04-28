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
#include "config.h"
#include "memory_store.h"
#include "dep_handler.h"
#include "message_manager.h"
#include "error_code.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <dlfcn.h>

class PluginManager {
public:
    PluginManager(Config &config, SafeQueue<Message> &handler_msg, SafeQueue<Message> &res_msg) : 
    config(config), handler_msg(handler_msg), res_msg(res_msg) {
        instance_run_handler.reset(new InstanceRunHandler(memory_store));
     }
    int run();
    void pre_load();
    void pre_enable();
    void init();
    void* get_data_buffer(std::string name);
private:
    void pre_load_plugin(PluginType type); 
    ErrorCode load_plugin(const std::string &path, PluginType type);
    ErrorCode remove(const std::string &name);
    ErrorCode query_all_plugins(std::string &res);
    ErrorCode query_plugin(const std::string &name, std::string &res);
    ErrorCode query_top(const std::string &name, std::string &res);
    ErrorCode query_all_tops(std::string &res);
    ErrorCode instance_enabled(std::string name);
    ErrorCode instance_disabled(std::string name);
    ErrorCode add_list(std::string &res);
    ErrorCode download(const std::string &name, std::string &res);
    std::string instance_dep_check(const std::string &name);
    template <typename T>
    int load_dl_instance(std::shared_ptr<Plugin> plugin, T **interface_list);
    template <typename T, typename U>
    void save_instance(std::shared_ptr<Plugin> plugin, T *interface_list, int len);
    bool load_instance(std::shared_ptr<Plugin> plugin);
    void batch_load();
    void batch_remove();
    void update_instance_state();
    bool end_with(const std::string &s, const std::string &ending);
    std::string get_plugin_in_dir(const std::string &path);
private:
    std::unique_ptr<InstanceRunHandler> instance_run_handler;
    Config &config;
    SafeQueue<Message> &handler_msg;
    SafeQueue<Message> &res_msg;
    MemoryStore memory_store;
    DepHandler dep_handler;
    std::unordered_map<std::string, PluginType> plugin_types; 
    static const std::string COLLECTOR_TEXT;
    static const std::string SCENARIO_TEXT;
    static const std::string TUNE_TEXT; 
};

bool check_permission(std::string path, int mode);
bool file_exist(const std::string &file_name);
#endif 

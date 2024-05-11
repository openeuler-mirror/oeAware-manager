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
#include "plugin_manager.h"
#include "default_path.h"
#include "utils.h"
#include <iostream>
#include <unordered_set>
#include <dirent.h>
#include <sys/stat.h>

const std::string PluginManager::COLLECTOR_TEXT = "collector";
const std::string PluginManager::SCENARIO_TEXT = "scenario";
const std::string PluginManager::TUNE_TEXT = "tune";
const static int ST_MODE_MASK = 0777;

void PluginManager::init() {
    plugin_types[COLLECTOR_TEXT] = PluginType::COLLECTOR;
    plugin_types[SCENARIO_TEXT] = PluginType::SCENARIO;
    plugin_types[TUNE_TEXT] = PluginType::TUNE;
}

ErrorCode PluginManager::remove(const std::string &name) {
    if (!memory_store.is_plugin_exist(name)) {
        return ErrorCode::REMOVE_PLUGIN_NOT_EXIST;
    }
    std::shared_ptr<Plugin> plugin = memory_store.get_plugin(name);
    std::vector<std::string> instance_names;
    for (int i = 0; i < plugin->get_instance_len(); ++i) {
        std::shared_ptr<Instance> instance = plugin->get_instance(i);
        std::string iname = instance->get_name();
        if (instance->get_enabled()) {
            return ErrorCode::REMOVE_INSTANCE_IS_RUNNING;
        }
        if (dep_handler.have_dep(iname)) {
            return ErrorCode::REMOVE_INSTANCE_HAVE_DEP;
        }
        instance_names.emplace_back(iname);
    }
    for(auto &iname : instance_names) {
        memory_store.delete_instance(iname);
        dep_handler.del_node(iname);
    }
    memory_store.delete_plugin(name);
    update_instance_state();
    return ErrorCode::OK;
}
ErrorCode PluginManager::query_all_plugins(std::string &res) {
    std::vector<std::shared_ptr<Plugin>> all_plugins = memory_store.get_all_plugins();
    for (auto &p : all_plugins) {
        res += p->get_name() + "\n";
        for (int i = 0; i < p->get_instance_len(); ++i) {
            std::string info = p->get_instance(i)->get_info();
            res += "\t" + info + "\n";
        }
    }
    return ErrorCode::OK;
}

ErrorCode PluginManager::query_plugin(const std::string &name, std::string &res) {
    if (!memory_store.is_plugin_exist(name)) {
        return ErrorCode::QUERY_PLUGIN_NOT_EXIST;
    }
    std::shared_ptr<Plugin> plugin = memory_store.get_plugin(name);
    res += name + "\n";
    for (int i = 0; i < plugin->get_instance_len(); ++i) {
        std::string info = plugin->get_instance(i)->get_info();
        res += "\t" + info + "\n";
    }
    return ErrorCode::OK;
}

template <typename T>
int PluginManager::load_dl_instance(std::shared_ptr<Plugin> plugin, T **interface_list) {
    int (*get_instance)(T**) = (int(*)(T**))dlsym(plugin->get_handler(), "get_instance");
    if (get_instance == nullptr) {
        ERROR("[PluginManager] dlsym error!\n");
        return -1;
    }
    int len = get_instance(interface_list);
    DEBUG("[PluginManager] dl loaded! ");
    return len;
}

template<typename T>
std::vector<std::string> get_dep(T *interface) {
    int dep_len = 0;
    char *deps = interface->get_dep();
    std::vector<std::string> res;
    std::string dep;
    for (int i = 0; deps[i] != 0; ++i) {
        if (deps[i] != '-') {
            dep += deps[i];
        } else {  
            res.emplace_back(dep);
            dep = "";
        }
    }
    if (!dep.empty()) res.emplace_back(dep);
    return res;
}

template<typename T, typename U>
void PluginManager::save_instance(std::shared_ptr<Plugin> plugin, T *interface_list, int len) {
    if (interface_list == nullptr) return;
    for (int i = 0; i < len; ++i) {
        T *interface = interface_list + i;
        std::shared_ptr<Instance> instance = std::make_shared<U>();
        std::string name = interface->get_name();
        instance->set_name(name);
        instance->set_plugin_name(plugin->get_name());
        instance->set_type(plugin->get_type());
        instance->set_enabled(false);
        if (plugin->get_type() == PluginType::COLLECTOR) {
            DEBUG("[PluginManager] add node");
            dep_handler.add_node(name); 
        } else {
            dep_handler.add_node(name, get_dep<T>(interface));
        }
        instance->set_state(dep_handler.get_node_state(name));
        (std::dynamic_pointer_cast<U>(instance))->set_interface(interface);
        DEBUG("[PluginManager] Instance: " << name.c_str());
        memory_store.add_instance(name, instance);
        plugin->add_instance(instance);    
    }
}

bool PluginManager::load_instance(std::shared_ptr<Plugin> plugin) {
    int len = 0;
    DEBUG("plugin: " << plugin->get_name());
    switch (plugin->get_type()) {
        case PluginType::COLLECTOR: {
            CollectorInterface *interface_list = nullptr;
            len = load_dl_instance<CollectorInterface>(plugin, &interface_list);
            if (len == -1) return false;
            DEBUG("[PluginManager] save collect instance");
            save_instance<CollectorInterface, CollectorInstance>(plugin, interface_list, len);  
            break;
        }
        case PluginType::SCENARIO: {
            ScenarioInterface *interface_list = nullptr;
            len = load_dl_instance<ScenarioInterface>(plugin, &interface_list);
            if (len == -1) return false;
            save_instance<ScenarioInterface, ScenarioInstance>(plugin, interface_list, len);
            break;
        }
        case PluginType::TUNE:
            TuneInterface *interface_list = nullptr;
            len = load_dl_instance<TuneInterface>(plugin, &interface_list);
            if (len == -1) return false;
            save_instance<TuneInterface, TuneInstance>(plugin, interface_list, len);
            break;
    }
    update_instance_state();
    return true;
}

void PluginManager::update_instance_state() {
    std::vector<std::shared_ptr<Instance>> all_instances = memory_store.get_all_instances();
    for (auto &instance : all_instances) {
        if (dep_handler.get_node_state(instance->get_name())) {
            instance->set_state(true);
        } else {
            instance->set_state(false);
        }
    }
}

ErrorCode PluginManager::load_plugin(const std::string &name, PluginType type) {
    std::string plugin_path = get_path(type) + "/" + name;
    if (!file_exist(plugin_path)) {
        return ErrorCode::LOAD_PLUGIN_FILE_NOT_EXIST;
    }
    if (!end_with(name, ".so")) {
        return ErrorCode::LOAD_PLUGIN_FILE_IS_NOT_SO; 
    }
    if (!check_permission(plugin_path, S_IRUSR | S_IRGRP)) {
        return ErrorCode::LOAD_PLUGIN_FILE_PERMISSION_DEFINED;
    }
    if (memory_store.is_plugin_exist(name)) {
        return ErrorCode::LOAD_PLUGIN_EXIST;
    }
    const std::string dl_path = get_path(type) + '/' + name;
    std::shared_ptr<Plugin> plugin = std::make_shared<Plugin>(name, type);
    int error = plugin->load(dl_path);
    if (error) {
        return ErrorCode::LOAD_PLUGIN_DLOPEN_FAILED;
    } 
    if (!this->load_instance(plugin)) {
        return ErrorCode::LOAD_PLUGIN_DLSYM_FAILED;
    }
    memory_store.add_plugin(name, plugin);
    return ErrorCode::OK;
}

std::string generate_dot(MemoryStore &memory_store, const std::vector<std::vector<std::string>> &query) {
    std::string res;
    res += "digraph G {\n";
    std::unordered_map<std::string, std::unordered_set<std::string>> sub_graph;
    for (auto &vec : query) {
        std::shared_ptr<Instance> instance = memory_store.get_instance(vec[0]);
        sub_graph[instance->get_plugin_name()].insert(vec[0]);
        if (vec.size() == 1) {
            continue;
        }
        instance = memory_store.get_instance(vec[1]);
        sub_graph[instance->get_plugin_name()].insert(vec[1]);
        res += vec[0] + "->"  + vec[1] + ";";
    }
    int id = 0;
    for (auto &p : sub_graph) {
        res += "subgraph cluster_" + std::to_string(id) + " {\n";
        res += "node [style=filled];\n";
        res += "label = \"" + p.first + "\";\n";
        for (auto &i_name : p.second) {
            res += i_name + ";\n";
        }
        res += "}\n";
        id++;
    }
    res += "}";
    return res;
}

ErrorCode PluginManager::query_top(const std::string &name, std::string &res) {
    if (!memory_store.is_instance_exist(name)) {
        return ErrorCode::QUERY_DEP_NOT_EXIST;
    }
    DEBUG("[PluginManager] query top : " << name);
    std::vector<std::vector<std::string>> query;
    dep_handler.query_node(name, query);
    res = generate_dot(memory_store, query);
    return ErrorCode::OK;
}

ErrorCode PluginManager::query_all_tops(std::string &res) {
    std::vector<std::vector<std::string>> query;
    dep_handler.query_all_top(query);  
    DEBUG("[PluginManager] query size:" << query.size());
    res = generate_dot(memory_store, query);
    return ErrorCode::OK;
}

ErrorCode PluginManager::instance_enabled(std::string name) {
    if (!memory_store.is_instance_exist(name)) {
        return ErrorCode::ENABLE_INSTANCE_NOT_LOAD;
    }
    std::shared_ptr<Instance> instance = memory_store.get_instance(name);
    if (!instance->get_state()) {
        return ErrorCode::ENABLE_INSTANCE_UNAVAILABLE;
    }
    if (instance->get_enabled()) {
        return ErrorCode::ENABLE_INSTANCE_ALREADY_ENABLED;
    }
    std::vector<std::string> pre_dependencies = dep_handler.get_pre_dependencies(name);
    for (int i = pre_dependencies.size() - 1; i >= 0; --i) {
        instance = memory_store.get_instance(pre_dependencies[i]);
        if (instance->get_enabled()) {
            continue;
        }
        instance->set_enabled(true);
        instance_run_handler->recv_queue_push(InstanceRunMessage(RunType::ENABLED, instance));
        DEBUG("[PluginManager] " << instance->get_name() << " instance enabled.");
    }
    return ErrorCode::OK;
}

ErrorCode PluginManager::instance_disabled(std::string name) {
    if (!memory_store.is_instance_exist(name)) {
        return ErrorCode::DISABLE_INSTANCE_NOT_LOAD;
    }
    std::shared_ptr<Instance> instance = memory_store.get_instance(name);
    if (!instance->get_state()) {
        return ErrorCode::DISABLE_INSTANCE_UNAVAILABLE;
    }
    if (!instance->get_enabled()) {
        return ErrorCode::DISABLE_INSTANCE_ALREADY_DISABLED;
    }
    instance->set_enabled(false);
    instance_run_handler->recv_queue_push(InstanceRunMessage(RunType::DISABLED, instance));
    return ErrorCode::OK;
}

bool PluginManager::end_with(const std::string &s, const std::string &ending) {
    if (s.length() >= ending.length()) {
        return (s.compare(s.length() - ending.length(), ending.length(), ending) == 0);
    } else {
        return false;
    }
}

std::string PluginManager::get_plugin_in_dir(const std::string &path) {
    std::string res;
    struct stat s = {};
    lstat(path.c_str(), &s);
    if (!S_ISDIR(s.st_mode)) {
        return res;
    }
    struct dirent *filename = nullptr;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return res;
    }
    while ((filename = readdir(dir)) != nullptr) {
        if (end_with(std::string(filename->d_name), ".so")) {
            res += std::string(filename->d_name) + "\n";
        }
    }
    return res;
}

ErrorCode PluginManager::add_list(std::string &res) {
    auto plugin_list = config.get_plugin_list();
    res += "Supported Packages:\n";
    for (auto &p : plugin_list) {
        res += p.first + "\n";
    }   
    res += "Installed Plugins:\n";
    res += get_plugin_in_dir(DEFAULT_COLLECTOR_PATH);
    res += get_plugin_in_dir(DEFAULT_SCENARIO_PATH);
    res += get_plugin_in_dir(DEFAULT_TUNE_PATH);
    return ErrorCode::OK;
}

ErrorCode PluginManager::download(const std::string &name, std::string &res) {          
    if (!config.is_plugin_info_exist(name)) {
        return ErrorCode::DOWNLOAD_NOT_FOUND;
    }
    res += config.get_plugin_info(name).get_url();
    return ErrorCode::OK;
}

void PluginManager::pre_enable() {
    for (int i = 0; i < config.get_enable_list_size(); ++i) {
        EnableItem item = config.get_enable_list(i);
        if (item.get_enabled()) {
            std::string name = item.get_name();
            if (!memory_store.is_plugin_exist(name)) {
                WARN("[PluginManager] plugin " << name << " cannot be enabled, because it does not exist.");
                continue;
            }
            std::shared_ptr<Plugin> plugin = memory_store.get_plugin(name);
            for (int j = 0; j < plugin->get_instance_len(); ++j) {
                instance_enabled(plugin->get_instance(j)->get_name());
            }
        } else {
            for (int j = 0; j < item.get_instance_size(); ++j) {
                std::string name = item.get_instance_name(j);
                if (!memory_store.is_instance_exist(name)) {
                    WARN("[PluginManager] instance " << name << " cannot be enabled, because it does not exist.");
                    continue;
                }
                instance_enabled(name);
            }
        }
    }
}

void PluginManager::pre_load_plugin(PluginType type) {
    std::string path = get_path(type);
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (end_with(name, ".so")) {
            auto ret = load_plugin(name, type);
            if (ret != ErrorCode::OK) {
                WARN("[PluginManager] " << name << " plugin preload failed, because " << ErrorText::get_error_text(ret) << ".");
            } else {
                INFO("[PluginManager] " << name << " plugin loaded.");
            }
        }
    }
    closedir(dir);
}

void PluginManager::pre_load() {
    pre_load_plugin(PluginType::COLLECTOR);
    pre_load_plugin(PluginType::SCENARIO);
    pre_load_plugin(PluginType::TUNE);
    pre_enable();
}

void* PluginManager::get_data_buffer(std::string name) {
    std::shared_ptr<Instance> instance = memory_store.get_instance(name);
    switch (instance->get_type()) {
        case PluginType::COLLECTOR: {
            CollectorInterface *collector_interface = (std::dynamic_pointer_cast<CollectorInstance>(instance))->get_interface();
            return collector_interface->get_ring_buf();
        }
        case PluginType::SCENARIO: {
            ScenarioInterface *scenario_interface = (std::dynamic_pointer_cast<ScenarioInstance>(instance))->get_interface();
            return scenario_interface->get_ring_buf();
        }
        default:
            return nullptr;
    }
    return nullptr;
}

std::string PluginManager::instance_dep_check(const std::string &name) {
    std::shared_ptr<Plugin> plugin = memory_store.get_plugin(name);
    std::string res;
    for (int i = 0; i < plugin->get_instance_len(); ++i) {
        std::string instance_name = plugin->get_instance(i)->get_name();
        std::vector<std::vector<std::string>> query;
        dep_handler.query_node(instance_name, query);
        std::vector<std::string> lack;
        for (auto &item : query) {
            if (item.size() < 2) continue;
            if (!memory_store.is_instance_exist(item[1])) {
                lack.emplace_back(item[1]);
            }
        }
        if (!lack.empty()) {
            for (int i = 0; i < lack.size(); ++i) {
                res += "\t" + lack[i];
                if (i != lack.size() - 1) res += '\n';
            }
        }
    }
    return res;
}

// Check the file permission. The file owner is root.
bool check_permission(std::string path, int mode) {
    struct stat st;
    lstat(path.c_str(), &st);
    int cur_mode = (st.st_mode & ST_MODE_MASK);
    DEBUG("[PluginManager]" << path << " st_mode: " << cur_mode << ",st_gid: " << st.st_gid << ", st_uid: " << st.st_uid);
    if (st.st_gid || st.st_uid) return false;
    
    if (cur_mode != mode) return false;
    return true;
}

bool file_exist(const std::string &file_name) {
    std::ifstream file(file_name);
    return file.good();
}

int PluginManager::run() {
    instance_run_handler->run();
    while (true) {
        Message msg;
        Message res;
        this->handler_msg.wait_and_pop(msg);
        if (msg.get_opt() == Opt::SHUTDOWN) break;
        switch (msg.get_opt()) {
            case Opt::LOAD: {
                std::string plugin_name = msg.get_payload(0);
                PluginType type = plugin_types[msg.get_payload(1)];
                ErrorCode ret_code = load_plugin(plugin_name, type);
                if(ret_code == ErrorCode::OK) {
                    INFO("[PluginManager] " << plugin_name << " plugin loaded.");
                    res.set_opt(Opt::RESPONSE_OK);
                    std::string lack_dep = instance_dep_check(plugin_name);
                    if (!lack_dep.empty()) {
                        INFO("[PluginManager] " << plugin_name << " requires the following dependencies:\n" << lack_dep);
                        res.add_payload(lack_dep);
                    }
                } else {
                    WARN("[PluginManager] " << plugin_name << " " << ErrorText::get_error_text(ret_code)  << ".");
                    res.set_opt(Opt::RESPONSE_ERROR);
                    res.add_payload(ErrorText::get_error_text(ret_code));
                }
                break;
            }
            case Opt::REMOVE: {
                std::string name = msg.get_payload(0);
                ErrorCode ret_code = remove(name); 
                if (ret_code == ErrorCode::OK) {
                    INFO("[PluginManager] " << name << " plugin removed.");
                    res.set_opt(Opt::RESPONSE_OK);
                } else {
                    res.set_opt(Opt::RESPONSE_ERROR);
                    res.add_payload(ErrorText::get_error_text(ret_code));
                    INFO("[PluginManager] " << name << " " << ErrorText::get_error_text(ret_code) + ".");
                }
                break;
            }
            case Opt::QUERY_ALL: {
                std::string res_text;
                ErrorCode ret_code = query_all_plugins(res_text);
                if (ret_code == ErrorCode::OK) {
                    INFO("[PluginManager] query all plugins information.");
                    res.set_opt(Opt::RESPONSE_OK);
                    res.add_payload(res_text);
                } else {
                    WARN("[PluginManager] query all plugins failed, because " << ErrorText::get_error_text(ret_code) + ".");
                    res.set_opt(Opt::RESPONSE_ERROR);
                    res.add_payload(ErrorText::get_error_text(ret_code));
                }
                break;
            }
            case Opt::QUERY: {
                std::string res_text;
                std::string name = msg.get_payload(0);
                ErrorCode ret_code = query_plugin(name, res_text);
                if (ret_code == ErrorCode::OK) {
                    INFO("[PluginManager] " << name << " plugin query successfully.");
                    res.set_opt(Opt::RESPONSE_OK);
                    res.add_payload(res_text);
                } else {
                    WARN("[PluginManager] " << name << " " << ErrorText::get_error_text(ret_code) + ".");
                    res.set_opt(Opt::RESPONSE_ERROR);
                    res.add_payload(ErrorText::get_error_text(ret_code));
                }
                break;
            }
            case Opt::QUERY_TOP: {
                std::string res_text;
                std::string name = msg.get_payload(0);
                ErrorCode ret_code = query_top(name , res_text);
                if (ret_code == ErrorCode::OK) {
                    INFO("[PluginManager] query " << name << " instance dependencies.");
                    res.set_opt(Opt::RESPONSE_OK);
                    res.add_payload(res_text);
                } else {
                    WARN("[PluginManager] query  "<< name  << " instance dependencies failed, because " 
                    << ErrorText::get_error_text(ret_code) << ".");
                    res.set_opt(Opt::RESPONSE_ERROR);
                    res.add_payload(ErrorText::get_error_text(ret_code));
                }
                break;
            }
            case Opt::QUERY_ALL_TOP: {
                std::string res_text;
                ErrorCode ret_code = query_all_tops(res_text);
                if (ret_code == ErrorCode::OK) {
                    INFO("[PluginManager] query all instances dependencies.");
                    res.set_opt(Opt::RESPONSE_OK);
                    res.add_payload(res_text);
                } else {
                    WARN("[PluginManager] query all instances dependencies failed. because " 
                    << ErrorText::get_error_text(ret_code) << ".");
                    res.set_opt(Opt::RESPONSE_ERROR);
                    res.add_payload(ErrorText::get_error_text(ret_code));
                }
                break;
            }
            case Opt::ENABLED: {
                if (msg.get_payload_len() < 1) {
                    WARN("[PluginManager] enable opt need arg!");
                    res.add_payload("enable opt need arg!");
                    break;
                }
                std::string instance_name = msg.get_payload(0);
                ErrorCode ret_code = instance_enabled(instance_name);
                if (ret_code == ErrorCode::OK) {
                    INFO("[PluginManager] " << instance_name << " enabled successful.");
                    res.set_opt(Opt::RESPONSE_OK);
                } else {
                    WARN("[PluginManager] " << instance_name << " enabled failed. because " 
                    << ErrorText::get_error_text(ret_code) + ".");
                    res.set_opt(Opt::RESPONSE_ERROR);
                    res.add_payload(ErrorText::get_error_text(ret_code));
                }
                break;
            }
            case Opt::DISABLED: {
                if (msg.get_payload_len() < 1) {
                    WARN("[PluginManager] disable opt need arg!");
                    res.add_payload("disable opt need arg!");
                    break;
                }
                std::string instance_name = msg.get_payload(0);
                ErrorCode ret_code = instance_disabled(instance_name);
                if (ret_code == ErrorCode::OK) {
                    INFO("[PluginManager] " << instance_name << " disabled successful.");
                    res.set_opt(Opt::RESPONSE_OK);    
                } else {
                    WARN("[PluginManager] " << instance_name << " " << ErrorText::get_error_text(ret_code) << ".");
                    res.set_opt(Opt::RESPONSE_ERROR);
                    res.add_payload(ErrorText::get_error_text(ret_code));
                }
                break;
            }
            case Opt::LIST: {
                std::string res_text;
                ErrorCode ret_code = add_list(res_text);
                if (ret_code == ErrorCode::OK) {
                    INFO("[PluginManager] query plugin_list.");
                    res.set_opt(Opt::RESPONSE_OK);   
                    res.add_payload(res_text); 
                } else {
                    WARN("[PluginManager] query plugin_list failed, because " << ErrorText::get_error_text(ret_code) << ".");
                    res.set_opt(Opt::RESPONSE_ERROR);
                    res.add_payload(ErrorText::get_error_text(ret_code));
                }
                break;
            }
            case Opt::DOWNLOAD: {
                std::string res_text;
                std::string name = msg.get_payload(0);
                ErrorCode ret_code = download(name, res_text);
                if (ret_code == ErrorCode::OK) {
                    INFO("[PluginManager] download " << name << " from " << res_text << ".");
                    res.set_opt(Opt::RESPONSE_OK);
                    res.add_payload(res_text);
                } else {
                    WARN("[PluginManager] download " << name << " failed, because " << ErrorText::get_error_text(ret_code) + ".");
                    res.set_opt(Opt::RESPONSE_ERROR);
                    res.add_payload(ErrorText::get_error_text(ret_code));
                }
            }
            default:
                break;
        }
        if (msg.get_type() == MessageType::EXTERNAL)
            res_msg.push(res);
    }
    return 0;
}

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
#include <dirent.h>
#include <sys/stat.h>

const std::string PluginManager::COLLECTOR_TEXT = "collector";
const std::string PluginManager::SCENARIO_TEXT = "scenario";
const std::string PluginManager::TUNE_TEXT = "tune";
const static int ST_MODE_MASK = 0777;

void PluginManager::init(Config *config) {
    plugin_types[COLLECTOR_TEXT] = PluginType::COLLECTOR;
    plugin_types[SCENARIO_TEXT] = PluginType::SCENARIO;
    plugin_types[TUNE_TEXT] = PluginType::TUNE;
    this->config = config;
}

bool PluginManager::remove(const std::string &name) {
    if (!memory_store.is_plugin_exist(name)) return false;
    Plugin *plugin = memory_store.get_plugin(name);
    std::vector<std::string> instance_names;
    for (int i = 0; i < plugin->get_instance_len(); ++i) {
        Instance *instance = plugin->get_instance(i);
        std::string iname = instance->get_name();
        if (dep_handler->have_dep(iname)) {
            return false;
        }
        instance_names.emplace_back(iname);
    }
    for(auto &iname : instance_names) {
        memory_store.delete_instance(iname);
        dep_handler->del_node(iname);
    }
    memory_store.delete_plugin(name);
    update_instance_state();
    return true;
}
bool PluginManager::query_all_plugins(Message &res) {
    std::vector<Plugin*> all_plugins = memory_store.get_all_plugins();
    for (auto &p : all_plugins) {
        res.add_payload(p->get_name());
        for (int i = 0; i < p->get_instance_len(); ++i) {
            std::string info = p->get_instance(i)->get_info();
            res.add_payload("   " + info);
        }
    }
    return 1;
}

bool PluginManager::query_plugin(std::string name, Message &res) {
    if (!memory_store.is_plugin_exist(name)) {
        res.add_payload("no such plugin!");
        return true;
    }
    Plugin *plugin = memory_store.get_plugin(name);
    res.add_payload(name);
    for (int i = 0; i < plugin->get_instance_len(); ++i) {
        std::string info = plugin->get_instance(i)->get_info();
        res.add_payload("   " + info);
    }
    return true;
}

template <typename T>
int PluginManager::load_dl_instance(Plugin *plugin, T **interface_list) {
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
void PluginManager::save_instance(Plugin *plugin, T *interface_list, int len) {
    if (interface_list == nullptr) return;
    for (int i = 0; i < len; ++i) {
        T *interface = interface_list + i;
        Instance *instance = new U();
        std::string name = interface->get_name();
        instance->set_name(name);
        instance->set_plugin_name(plugin->get_name());
        instance->set_type(plugin->get_type());
        instance->set_enabled(false);
        if (plugin->get_type() == PluginType::COLLECTOR) {
            DEBUG("[PluginManager] add node");
            dep_handler->add_node(name); 
        } else {
            dep_handler->add_node(name, get_dep<T>(interface));
        }
        instance->set_state(dep_handler->get_node_state(name));
        ((U*)instance)->set_interface(interface);
        DEBUG("[PluginManager] Instance: " << name.c_str());
        memory_store.add_instance(name, instance);
        plugin->add_instance(instance);    
    }
}

bool PluginManager::load_instance(Plugin *plugin) {
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
    std::vector<Instance*> all_instances = memory_store.get_all_instances();
    for (auto &instance : all_instances) {
        if (dep_handler->get_node_state(instance->get_name())) {
            instance->set_state(true);
        } else {
            instance->set_state(false);
        }
    }
}

bool PluginManager::load_plugin(const std::string name, PluginType type) {
    if (memory_store.is_plugin_exist(name)) {
        WARN("[PluginManager] " << name << " already loaded!");
        return false;
    }
    const std::string dl_path = get_path(type) + '/' + name;
    Plugin *plugin = new Plugin(name, type);
    int error = plugin->load(dl_path);
    if (error) {
        WARN("[PluginManager] " << name << " load error!");
        return false;
    } 
    if (!this->load_instance(plugin)) {
        delete plugin;
        return false;
    }
    memory_store.add_plugin(name, plugin);
    return true;
}

std::string generate_dot(MemoryStore &memory_store, const std::vector<std::vector<std::string>> &query) {
    std::string res;
    res += "digraph G {\n";
    std::unordered_map<std::string, std::vector<std::string>> sub_graph;
    for (auto &vec : query) {
        Instance *instance = memory_store.get_instance(vec[0]);
        sub_graph[instance->get_plugin_name()].emplace_back(vec[0]);
        if (vec.size() == 1) {
            continue;
        }
        instance = memory_store.get_instance(vec[1]);
        sub_graph[instance->get_plugin_name()].emplace_back(vec[1]);
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

bool PluginManager::query_top(std::string name, Message &res) {
    DEBUG("[PluginManager] query top : " << name);
    std::vector<std::vector<std::string>> query;
    dep_handler->query_node(name, query);
    if (query.empty()) {
        res.add_payload("Instance not available!");
        return false;
    }
    std::string dot_text = generate_dot(memory_store, query);
    res.add_payload(dot_text);
    return true;
}

bool PluginManager::query_all_tops(Message &res) {
    std::vector<std::vector<std::string>> query;
    dep_handler->query_all_top(query);  
    DEBUG("[PluginManager] query size:" << query.size());
    if (query.empty()) {
        res.add_payload("No instance available!");
        return false;
    }
    std::string dot_text = generate_dot(memory_store, query);
    res.add_payload(dot_text);
    return true;
}

bool PluginManager::instance_enabled(std::string name) {
    if (!memory_store.is_instance_exist(name)) {
        WARN("[PluginManager] " << name << " instance can't load!");
        return false;
    }
    Instance *instance = memory_store.get_instance(name);
    if (!instance->get_state()) {
        WARN("[PluginManager] " << name << " instance is unavailable, lacking dependencies!");
        return false;
    }
    if (instance->get_enabled()) {
        WARN("[PluginManager] " << name << " instance was enabled!");
        return false;
    }
    std::vector<std::string> pre_dependencies = dep_handler->get_pre_dependencies(name);
    for (int i = pre_dependencies.size() - 1; i >= 0; --i) {
        instance = memory_store.get_instance(pre_dependencies[i]);
        if (instance->get_enabled()) {
            continue;
        }
        instance->set_enabled(true);
        instance_run_handler->recv_queue_push(InstanceRunMessage(RunType::ENABLED, instance));
        INFO("[PluginManager] " << name << " instance enabled!");
    }
    return true;
}

bool PluginManager::instance_disabled(std::string name) {
    if (!memory_store.is_instance_exist(name)) {
        WARN("[PluginManager] " << name << " instance can't load!");
        return false;
    }
    Instance *instance = memory_store.get_instance(name);
    if (!instance->get_state()) {
        WARN("[PluginManager] " << name << " instance is unavailable, lacking dependencies!");
        return false;
    }
    if (!instance->get_enabled()) {
        WARN("[PluginManager] " << name << " instance was disabled!");
        return false;
    }
    instance->set_enabled(false);
    instance_run_handler->recv_queue_push(InstanceRunMessage(RunType::DISABLED, instance));
    INFO("[PluginManager] " << name << " instance disabled!");
    return true;
}

static bool end_with(const std::string &s, const std::string &ending) {
    if (s.length() >= ending.length()) {
        return (s.compare(s.length() - ending.length(), ending.length(), ending) == 0);
    } else {
        return false;
    }
}

static std::string get_plugin_in_dir(const std::string &path) {
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

void PluginManager::add_list(Message &res) {
    std::string list_text;
    list_text += "Download Packages:\n";
    for (int i = 0; i < config->get_plugin_list_size(); ++i) {
        PluginInfo info = config->get_plugin_list(i);
        list_text += info.get_name() + "\n";
    }   
    list_text += "Installed Plugins:\n";
    list_text += get_plugin_in_dir(DEFAULT_COLLECTOR_PATH);
    list_text += get_plugin_in_dir(DEFAULT_SCENARIO_PATH);
    list_text += get_plugin_in_dir(DEFAULT_TUNE_PATH);
    res.add_payload(list_text);
}

void PluginManager::pre_enable() {
    for (int i = 0; i < config->get_enable_list_size(); ++i) {
        EnableItem item = config->get_enable_list(i);
        if (item.get_enabled()) {
            std::string name = item.get_name();
            if (!memory_store.is_plugin_exist(name)) {
                WARN("[PluginManager] plugin " << name << " cannot be enabled, because it does not exist.");
                continue;
            }
            Plugin *plugin = memory_store.get_plugin(name);
            for (int j = 0; j < plugin->get_instance_len(); ++j) {
                instance_enabled(plugin->get_instance(i)->get_name());
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
            Message msg;
            load_plugin(name, type);
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

static bool check_load_msg(Message &msg, std::unordered_map<std::string, PluginType> &plugin_types) {
    return msg.get_payload_len() == 2 && plugin_types.count(msg.get_payload(1));
}

void* PluginManager::get_data_buffer(std::string name) {
    Instance *instance = memory_store.get_instance(name);
    switch (instance->get_type()) {
        case PluginType::COLLECTOR: {
            CollectorInterface *collector_interface = ((CollectorInstance*)instance)->get_interface();
            return collector_interface->get_ring_buf();
        }
        case PluginType::SCENARIO: {
            ScenarioInterface *scenario_interface = ((ScenarioInstance*)instance)->get_interface();
            return scenario_interface->get_ring_buf();
        }
        default:
            return nullptr;
    }
    return nullptr;
}

void PluginManager::instance_dep_check(std::string name, Message &res) {
    Plugin *plugin = memory_store.get_plugin(name);
    for (int i = 0; i < plugin->get_instance_len(); ++i) {
        std::string instance_name = plugin->get_instance(i)->get_name();
        std::vector<std::vector<std::string>> query;
        dep_handler->query_node(instance_name, query);
        std::vector<std::string> lack;
        for (auto &item : query) {
            if (item.size() < 2) continue;
            if (!memory_store.is_instance_exist(item[1])) {
                lack.emplace_back(item[1]);
            }
        }
        if (!lack.empty()) {
            std::string info = instance_name + " needed the following dependencies:";
            for (auto &dep : lack) {
                info += "\n     " + dep;
            }
            res.add_payload(info);
        }
    }
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

static bool file_exist(const std::string &file_name) {
    std::ifstream file(file_name);
    return file.good();
}

int PluginManager::run() {
    instance_run_handler->set_memory_store(&memory_store);
    instance_run_handler->run();
    while (true) {
        Message msg;
        Message res;
        res.set_opt(Opt::RESPONSE);
        this->handler_msg->wait_and_pop(msg);
        if (msg.get_opt() == Opt::SHUTDOWN) break;
        switch (msg.get_opt()) {
            case Opt::LOAD: {
                if (!check_load_msg(msg, plugin_types)) {
                    WARN("[PluginManager] args error!");
                    res.add_payload("args error!");
                    break;
                }
                std::string plugin_name = msg.get_payload(0);
                PluginType type = plugin_types[msg.get_payload(1)];
                std::string plugin_path = get_path(type) + "/" + plugin_name;
                if (!end_with(plugin_name, ".so")) break;
                if (!file_exist(plugin_path)) {
                    WARN("[PluginManager] plugin " << plugin_name << " does not exist!");
                    res.add_payload("plugin does not exist!");
                    break;
                }
                if (!check_permission(plugin_path, S_IRUSR | S_IRGRP)) {
                    WARN("[PluginManager] plugin " << plugin_name << " does not have the execute permission!");
                    res.add_payload("does not have the execute permission!");
                    break;
                }
                if(this->load_plugin(plugin_name, type)) {
                    INFO("[PluginManager] plugin " << plugin_name << " loaded.");
                    res.add_payload("plugin load succeed!");
                    instance_dep_check(plugin_name, res);
                    DEBUG("[PluginManager] instance dependency checked!");
                } else {
                    INFO("[PluginManager] plugin " << plugin_name << " load error!");
                    res.add_payload("plugin load failed!");
                }
                break;
            }
            case Opt::REMOVE: {
                std::string name = msg.get_payload(0);
                if (remove(name)) {
                    res.add_payload(name + " removed!");
                    INFO("[PluginManager] " << name << " removed!");
                } else {
                   res.add_payload(name + " remove failed!");
                }
                break;
            }
            case Opt::QUERY_ALL: {
                query_all_plugins(res);
                break;
            }
            case Opt::QUERY: {
                query_plugin(msg.get_payload(0), res);
                break;
            }
            case Opt::QUERY_TOP: {
                query_top(msg.get_payload(0), res);
                break;
            }
            case Opt::QUERY_ALL_TOP: {
                query_all_tops(res);
                break;
            }
            case Opt::ENABLED: {
                if (msg.get_payload_len() < 1) {
                    WARN("[PluginManager] enable opt need arg!");
                    res.add_payload("enable opt need arg!");
                    break;
                }
                std::string instance_name = msg.get_payload(0);
                if (instance_enabled(instance_name)) {
                    res.add_payload("instance enabled!");
                } else {
                    res.add_payload("instance enabled failed!");
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
                if (instance_disabled(instance_name)) {
                    res.add_payload("instance disabled!");
                } else {
                    res.add_payload("instance disabled failed!");
                }
                break;
            }
            case Opt::LIST: {
                add_list(res);
                break;
            }
            case Opt::DOWNLOAD: {
                std::string name = msg.get_payload(0);
                std::string url = "";
                std::string type = "";
                for (int i = 0; i < config->get_plugin_list_size(); ++i) {
                    PluginInfo info = config->get_plugin_list(i);
                    if (info.get_name() == name) {
                        url = info.get_url();
                        break;
                    }
                }             
                if (url.empty()) {
                    WARN("[PluginManager] unable to find a match: " << name);
                    res.set_state_code(HEADER_STATE_FAILED);
                    res.add_payload("unable to find a match: " + name);
                    break;
                }
                res.add_payload(url);
                INFO("[PluginManager] download " << name << " from " << url << ".");
            }
            default:
                break;
        }
        if (msg.get_type() == MessageType::EXTERNAL)
            res_msg->push(res);
    }
    return 0;
}

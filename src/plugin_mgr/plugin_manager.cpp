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
#include <dirent.h>

const static int ST_MODE_MASK = 0777;


void PluginManager::init(std::shared_ptr<Config> config, std::shared_ptr<SafeQueue<Message>> handler_msg,
    std::shared_ptr<SafeQueue<Message>> res_msg) {
    this->config = config;
    this->handler_msg = handler_msg;
    this->res_msg = res_msg;
    instance_run_handler.reset(new InstanceRunHandler(memory_store));
    pre_load();
}

ErrorCode PluginManager::remove(const std::string &name) {
    if (!memory_store.is_plugin_exist(name)) {
        return ErrorCode::REMOVE_PLUGIN_NOT_EXIST;
    }
    std::shared_ptr<Plugin> plugin = memory_store.get_plugin(name);
    std::vector<std::string> instance_names;
    for (size_t i = 0; i < plugin->get_instance_len(); ++i) {
        std::shared_ptr<Instance> instance = plugin->get_instance(i);
        std::string iname = instance->get_name();
        if (instance->get_enabled()) {
            return ErrorCode::REMOVE_INSTANCE_IS_RUNNING;
        }
        if (memory_store.have_dep(iname)) {
            return ErrorCode::REMOVE_INSTANCE_HAVE_DEP;
        }
        instance_names.emplace_back(iname);
    }
    for(auto &iname : instance_names) {
        memory_store.delete_instance(iname);
    }
    memory_store.delete_plugin(name);
    return ErrorCode::OK;
}

ErrorCode PluginManager::query_all_plugins(std::string &res) {
    std::vector<std::shared_ptr<Plugin>> all_plugins = memory_store.get_all_plugins();
    for (auto &p : all_plugins) {
        res += p->get_name() + "\n";
        for (size_t i = 0; i < p->get_instance_len(); ++i) {
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
    for (size_t i = 0; i < plugin->get_instance_len(); ++i) {
        std::string info = plugin->get_instance(i)->get_info();
        res += "\t" + info + "\n";
    }
    return ErrorCode::OK;
}

int PluginManager::load_dl_instance(std::shared_ptr<Plugin> plugin, Interface **interface_list) {
    int (*get_instance)(Interface**) = (int(*)(Interface**))dlsym(plugin->get_handler(), "get_instance");
    if (get_instance == nullptr) {
        ERROR("[PluginManager] dlsym error!\n");
        return -1;
    }
    int len = get_instance(interface_list);
    DEBUG("[PluginManager] dl loaded! ");
    return len;
}

void PluginManager::save_instance(std::shared_ptr<Plugin> plugin, Interface *interface_list, int len) {
    if (interface_list == nullptr) return;
    for (int i = 0; i < len; ++i) {
        Interface *interface = interface_list + i;
        std::shared_ptr<Instance> instance = std::make_shared<Instance>();
        std::string name = interface->get_name();
        instance->set_interface(interface);
        instance->set_name(name);
        instance->set_plugin_name(plugin->get_name());
        instance->set_enabled(false);
        DEBUG("[PluginManager] Instance: " << name.c_str());
        memory_store.add_instance(instance);
        plugin->add_instance(instance);
    }
}

bool PluginManager::load_instance(std::shared_ptr<Plugin> plugin) {
    int len = 0;
    DEBUG("plugin: " << plugin->get_name());
    Interface *interface_list = nullptr;
    len = load_dl_instance(plugin, &interface_list);
    if (len < 0) {
        return false;
    }
    save_instance(plugin, interface_list, len);
    return true;
}

ErrorCode PluginManager::load_plugin(const std::string &name) {
    std::string plugin_path = get_path() + "/" + name;
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
    std::shared_ptr<Plugin> plugin = std::make_shared<Plugin>(name);
    int error = plugin->load(plugin_path);
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
    res += "    rankdir = TB\n";
    res += "    ranksep = 1\n";
    std::unordered_map<std::string, std::unordered_set<std::string>> sub_graph;
    for (auto &vec : query) {
        std::shared_ptr<Instance> instance = memory_store.get_instance(vec[0]);
        sub_graph[instance->get_plugin_name()].insert(vec[0]);
        if (vec.size() == 1) {
            continue;
        }
        if (memory_store.is_instance_exist(vec[1])) {
            instance = memory_store.get_instance(vec[1]);
            sub_graph[instance->get_plugin_name()].insert(vec[1]);
        } else {
            res += "    " + vec[1] + "[label=\"(missing)\\n" + vec[1] + "\", fontcolor=red];\n";
        }
        res += "    " + vec[0] + "->"  + vec[1] + ";\n";
    }
    int id = 0;
    for (auto &p : sub_graph) {
        res += "    subgraph cluster_" + std::to_string(id) + " {\n";
        res += "        node [style=filled];\n";
        res += "        label = \"" + p.first + "\";\n";
        for (auto &i_name : p.second) {
            res += "        " + i_name + ";\n";
        }
        res += "    }\n";
        id++;
    }
    res += "}";
    return res;
}

ErrorCode PluginManager::query_dependency(const std::string &name, std::string &res) {
    if (!memory_store.is_instance_exist(name)) {
        return ErrorCode::QUERY_DEP_NOT_EXIST;
    }
    DEBUG("[PluginManager] query top : " << name);
    std::vector<std::vector<std::string>> query;
    memory_store.query_node_dependency(name, query);
    res = generate_dot(memory_store, query);
    return ErrorCode::OK;
}

ErrorCode PluginManager::query_all_dependencies(std::string &res) {
    std::vector<std::vector<std::string>> query;
    memory_store.query_all_dependencies(query);  
    DEBUG("[PluginManager] query size:" << query.size());
    res = generate_dot(memory_store, query);
    return ErrorCode::OK;
}

ErrorCode PluginManager::instance_enabled(const std::string &name) {
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
    std::vector<std::string> pre_dependencies = memory_store.get_pre_dependencies(name);
    std::vector<std::shared_ptr<Instance>> new_enabled;
    bool enabled = true;
    for (int i = pre_dependencies.size() - 1; i >= 0; --i) {
        instance = memory_store.get_instance(pre_dependencies[i]);
        if (instance->get_enabled()) {
            continue;
        }
        new_enabled.emplace_back(instance);
        if (!instance->get_interface()->enable()) {
            enabled = false;
            WARN("[PluginManager] " << instance->get_name() << " instance enabled failed, because instance init environment failed.");
            break;
        }
    }
    if (enabled) {
        for (int i = pre_dependencies.size() - 1; i >= 0; --i) {
            instance = memory_store.get_instance(pre_dependencies[i]);
            if (instance->get_enabled()) {
                continue;
            }
            instance->set_enabled(true);
            instance_run_handler->recv_queue_push(InstanceRunMessage(RunType::ENABLED, instance));
        }
        return ErrorCode::OK;
    } else {
        for (auto ins : new_enabled) {
            ins->get_interface()->disable();
        }
        return ErrorCode::ENABLE_INSTANCE_ENV;
    }
}

ErrorCode PluginManager::instance_disabled(const std::string &name) {
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
    auto plugin_list = config->get_plugin_list();
    res += "Supported Packages:\n";
    for (auto &p : plugin_list) {
        res += p.first + "\n";
    }   
    res += "Installed Plugins:\n";
    res += get_plugin_in_dir(DEFAULT_PLUGIN_PATH);
    return ErrorCode::OK;
}

ErrorCode PluginManager::download(const std::string &name, std::string &res) {          
    if (!config->is_plugin_info_exist(name)) {
        return ErrorCode::DOWNLOAD_NOT_FOUND;
    }
    res += config->get_plugin_info(name).get_url();
    return ErrorCode::OK;
}

void PluginManager::pre_enable() {
    for (size_t i = 0; i < config->get_enable_list_size(); ++i) {
        EnableItem item = config->get_enable_list(i);
        if (item.get_enabled()) {
            std::string name = item.get_name();
            if (!memory_store.is_plugin_exist(name)) {
                WARN("[PluginManager] plugin " << name << " cannot be enabled, because it does not exist.");
                continue;
            }
            std::shared_ptr<Plugin> plugin = memory_store.get_plugin(name);
            for (size_t j = 0; j < plugin->get_instance_len(); ++j) {
                instance_enabled(plugin->get_instance(j)->get_name());
            }
        } else {
            for (size_t j = 0; j < item.get_instance_size(); ++j) {
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

void PluginManager::pre_load_plugin() {
    std::string path = get_path();
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (end_with(name, ".so")) {
            auto ret = load_plugin(name);
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
    pre_load_plugin();
    pre_enable();
}

const void* PluginManager::get_data_buffer(const std::string &name) {
    std::shared_ptr<Instance> instance = memory_store.get_instance(name);
    return instance->get_interface()->get_ring_buf();
}

std::string PluginManager::instance_dep_check(const std::string &name) {
    std::shared_ptr<Plugin> plugin = memory_store.get_plugin(name);
    std::string res;
    for (size_t i = 0; i < plugin->get_instance_len(); ++i) {
        std::string instance_name = plugin->get_instance(i)->get_name();
        std::vector<std::vector<std::string>> query;
        memory_store.query_node_dependency(instance_name, query);
        std::vector<std::string> lack;
        for (auto &item : query) {
            if (item.size() < 2) continue;
            if (!memory_store.is_instance_exist(item[1])) {
                lack.emplace_back(item[1]);
            }
        }
        if (!lack.empty()) {
            for (size_t j = 0; j < lack.size(); ++j) {
                res += "\t" + lack[j];
                if (j != lack.size() - 1) res += '\n';
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
        this->handler_msg->wait_and_pop(msg);
        if (msg.get_opt() == Opt::SHUTDOWN) break;
        switch (msg.get_opt()) {
            case Opt::LOAD: {
                std::string plugin_name = msg.get_payload(0);
                ErrorCode ret_code = load_plugin(plugin_name);
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
            case Opt::QUERY_DEP: {
                std::string res_text;
                std::string name = msg.get_payload(0);
                ErrorCode ret_code = query_dependency(name , res_text);
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
            case Opt::QUERY_ALL_DEPS: {
                std::string res_text;
                ErrorCode ret_code = query_all_dependencies(res_text);
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
            res_msg->push(res);
    }
    return 0;
}

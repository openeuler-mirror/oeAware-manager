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
#ifndef PLUGIN_MGR_CONFIG_H
#define PLUGIN_MGR_CONFIG_H

#include "plugin.h"
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include <sys/stat.h>

static int log_levels[] = {0, 10000, 20000, 30000, 40000, 50000, 60000};

class PluginInfo {
public:
    PluginInfo(const std::string &name, const std::string &description, const std::string &url) : name(name), description(description), url(url) { }
    std::string get_name() const {
        return this->name;
    }
    std::string get_description() const {
        return this->description;
    }
    std::string get_url() const {
        return this->url;
    }
    bool operator ==(const PluginInfo &other) const {
        return name == other.get_name();
    }
private:
    std::string name;
    std::string description;
    std::string url;
};

namespace std {
    template <>
    struct hash<PluginInfo> {
        size_t operator ()(const PluginInfo &obj) const {
            return hash<std::string>()(obj.get_name());
        }
    };
}

class EnableItem {
public:
    explicit EnableItem(const std::string &name) : name(name), enabled(false) { }
    void set_enabled(bool enabled) {
        this->enabled = enabled;
    }
    bool get_enabled() const {
        return this->enabled;
    }
    void add_instance(std::string text) {
        this->instance.emplace_back(text);
    }
    size_t get_instance_size() const {
        return this->instance.size();
    }
    std::string get_instance_name(int i) {
        return this->instance[i];
    }
    std::string get_name() const {
        return this->name;
    }
private:
    std::vector<std::string> instance;
    std::string name;
    bool enabled;
};

class Config {
public:
    Config() {
        this->log_level = log_levels[2];
    }
    bool load(const std::string &path);
    int get_log_level() const {
        return this->log_level;
    }
    std::string get_log_type() const {
        return this->log_type;
    }
    std::string get_log_path() const {
        return this->log_path;
    }
    PluginInfo get_plugin_info(const std::string &name) const {
        return this->plugin_list.at(name);
    }
    std::unordered_map<std::string, PluginInfo> get_plugin_list() const {
        return this->plugin_list;
    }
    size_t get_plugin_list_size() const {
        return this->plugin_list.size();
    }
    bool is_plugin_info_exist(const std::string &name) {
        return this->plugin_list.count(name);
    }
    EnableItem get_enable_list(int i) const {
        return this->enable_list[i];
    }
    size_t get_enable_list_size() const {
        return this->enable_list.size();
    }
private:
    int log_level;
    std::string log_path;
    std::string log_type;
    std::unordered_map<std::string, PluginInfo> plugin_list;
    std::vector<EnableItem> enable_list;
};

std::string get_path();
bool create_dir(const std::string &path);

#endif
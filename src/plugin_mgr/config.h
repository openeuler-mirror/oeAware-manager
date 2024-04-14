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
#include <string>
#include <yaml-cpp/yaml.h>
#include <sys/stat.h>

static int log_levels[] = {0, 10000, 20000, 30000, 40000, 50000, 60000};

class PluginInfo {
public:
    PluginInfo(std::string name, std::string description, std::string url) : name(name), description(description), url(url) { }
    std::string get_name() const {
        return this->name;
    }
    std::string get_description() const {
        return this->description;
    }
    std::string get_url() const {
        return this->url;
    }
private:
    std::string name;
    std::string description;
    std::string url;
};

class EnableItem {
public:
    EnableItem(std::string name) : name(name) { }
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
    std::string get_name() const {
        return this->name;
    }
private:
    bool enabled;
    std::string name;
    std::vector<std::string> instance;
};

class Config {
public:
    Config() {
        this->log_level = log_levels[2];
    }
    bool load(const std::string path);
    int get_log_level() const {
        return this->log_level;
    }
    int get_schedule_cycle() const {
        return this->schedule_cycle;
    }
    std::string get_log_type() const {
        return this->log_type;
    }
    std::string get_log_path() const {
        return this->log_path;
    }
    PluginInfo get_plugin_list(int i) const {
        return this->plugin_list[i];
    }
    size_t get_plugin_list_size() const {
        return this->plugin_list.size();
    }
    EnableItem get_enable_list(int i) const {
        return this->enable_list[i];
    }
    size_t get_enable_list_size() const {
        return this->enable_list.size();
    }
private:
    int log_level;
    int schedule_cycle;
    std::string log_path;
    std::string log_type;
    std::vector<PluginInfo> plugin_list;
    std::vector<EnableItem> enable_list;
};

std::string get_path(PluginType type);
void create_dir(std::string path);

#endif
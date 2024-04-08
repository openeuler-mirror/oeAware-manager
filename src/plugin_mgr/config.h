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

#include <string>
#include <yaml-cpp/yaml.h>
#include <sys/stat.h>
#include "plugin.h"

static int log_levels[] = {0, 10000, 20000, 30000, 40000, 50000, 60000};
const std::string DEFAULT_COLLECTOR_PATH = "/usr/lib64/oeAware-plugin/collector";
const std::string DEFAULT_SCENARIO_PATH = "/usr/lib64/oeAware-plugin/scenario";
const std::string DEFAULT_TUNE_PATH = "/usr/lib64/oeAware-plugin/tune";
const std::string DEFAULT_RUN_PATH = "/var/run/oeAware";
const std::string DEFAULT_LOG_PATH = "/var/log/oeAware";

class PluginInfo {
public:
    PluginInfo(std::string name, std::string type, std::string description, std::string url) : name(name), type(type), description(description), url(url) { }
    std::string get_name() {
        return this->name;
    }
    std::string get_type() {
        return this->type;
    }
    std::string get_description() {
        return this->description;
    }
    std::string get_url() {
        return this->url;
    }
private:
    std::string name;
    std::string type;
    std::string description;
    std::string url;
};

class Config {
public:
    Config() {
        this->log_level = log_levels[2];
    }

    bool load(const std::string path);

    int get_log_level() {
        return this->log_level;
    }
    int get_schedule_cycle() {
        return this->schedule_cycle;
    }
    std::string get_log_type() {
        return this->log_type;
    }
    std::string get_log_path() {
        return this->log_path;
    }
    PluginInfo get_preload_plugins(int i) {
        return this->preload_plugins[i];
    }
    int get_preload_plugins_size() {
        return this->preload_plugins.size();
    }
private:
    int log_level;
    int schedule_cycle;
    std::string log_path;
    std::string log_type;
    std::vector<PluginInfo> preload_plugins;
};
std::string get_path(PluginType type);
#endif
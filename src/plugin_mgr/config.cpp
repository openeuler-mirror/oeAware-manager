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
#include "config.h"
#include "default_path.h"
#include <iostream>
#include <unistd.h>

void create_dir(std::string path) {
    if (access(path.c_str(), F_OK) == -1) {
        mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IROTH);
    }
}

bool Config::load(const std::string path) {
    YAML::Node node;
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return false;
    }
    try {
        node = YAML::LoadFile(path);
        if (!node.IsMap()) return false;
        if (!node["log_path"].IsNull()) {
            this->log_path = node["log_path"].as<std::string>();
        }
        if (!node["log_level"].IsNull()) {
            this->log_level =  log_levels[node["log_level"].as<int>()];
        }
        if (!node["plugin_list"].IsNull()) {
            YAML::Node plugin_list = node["plugin_list"];
            if (plugin_list.IsSequence()) {
                for (int i = 0; i < plugin_list.size(); ++i) {
                    std::string name = plugin_list[i]["name"].as<std::string>();
                    std::string description = plugin_list[i]["description"].as<std::string>();
                    std::string url = plugin_list[i]["url"].as<std::string>();
                    this->plugin_list.emplace_back(PluginInfo(name, description, url));
                }
            } else {
                std::cerr << "Error: 'plugin_list' is not a sequence" << '\n';
            }
        }
        if (!node["enable_list"].IsNull()) {
            YAML::Node enable_list = node["enable_list"];
            if (enable_list.IsSequence()) {
                for (int i = 0; i < enable_list.size(); ++i) {
                    YAML::Node instances = enable_list[i]["instances"];
                    std::string name = enable_list[i]["name"].as<std::string>();
                    EnableItem enable_item(name);
                    if (instances.IsNull()) {
                        enable_item.set_enabled(true);
                    } else if (instances.IsSequence()) {
                        for (int j = 0; j < instances.size(); ++j) {
                            std::string i_name = instances[j]["name"].as<std::string>();
                            enable_item.add_instance(i_name);
                        }
                    } else {
                        continue;
                    }
                    this->enable_list.emplace_back(enable_item);
                }
            }
        }
    }
    catch (const YAML::Exception& e) {
        std::cerr << e.what() << '\n';
        return false;
    }
    create_dir(this->log_path);
    return true;
}

std::string get_path(PluginType type) {
    switch (type) {
        case PluginType::COLLECTOR:{
            return DEFAULT_COLLECTOR_PATH;
        }
        case PluginType::SCENARIO: {
            return DEFAULT_SCENARIO_PATH;
        }
        case PluginType::TUNE: {
            return DEFAULT_TUNE_PATH;
        }
    }
    return "";
}
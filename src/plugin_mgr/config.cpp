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
#include <unistd.h>
#include <iostream>

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
        if (!node["plugins"].IsNull()) {
            YAML::Node plugins = node["plugins"];
            if (plugins.IsSequence()) {
                for (int i = 0; i < plugins.size(); ++i) {
                    std::string name = plugins[i]["name"].as<std::string>();
                    std::string type = plugins[i]["type"].as<std::string>();
                    std::string description = plugins[i]["description"].as<std::string>();
                    std::string url = plugins[i]["url"].as<std::string>();
                    this->preload_plugins.emplace_back(PluginInfo(name, type, description, url));
                }
            } else {
                std::cerr << "Error: 'plugins' is not a sequence" << '\n';
            }
        }
    }
    catch (const YAML::Exception& e) {
        
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
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

bool create_dir(const std::string &path) {
    size_t  pos = 0;
    do {
        pos = path.find_first_of("/", pos + 1);
        std::string sub_path = path.substr(0, pos);
        struct stat buffer;
        if (stat(sub_path.c_str(), &buffer) == 0) {
            continue;
        }
        if (mkdir(sub_path.c_str(), S_IRWXU | S_IRWXG) != 0) {
            return false;
        }
    } while (pos != std::string::npos);
    return true;
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
                    PluginInfo info(name, description, url);
                    if (name.empty()) {
                        std::cerr << "Warn: " << name << " url is empty.\n";
                        continue;
                    }
                    if (this->plugin_list.count(name)) {
                        std::cerr << "Warn: duplicate " << name << " in plugin_list.\n";
                        continue;
                    }
                    this->plugin_list.insert(std::make_pair(name, info));
                }
            } else {
                std::cerr << "Error: 'plugin_list' is not a sequence" << '\n';
            }
        }
        if (!node["enable_list"].IsNull()) {
            YAML::Node enable_list = node["enable_list"];
            if (enable_list.IsSequence()) {
                for (int i = 0; i < enable_list.size(); ++i) {
                    YAML::Node plugin = enable_list[i]["name"];
                    std::string name = enable_list[i]["name"].as<std::string>();
                    EnableItem enable_item(name);
                    if (plugin.IsScalar()) {
                        enable_item.set_enabled(true);
                    } else if (plugin.IsSequence()) {
                        for (int j = 0; j < plugin.size(); ++j) {
                            std::string i_name = plugin[j].as<std::string>();
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
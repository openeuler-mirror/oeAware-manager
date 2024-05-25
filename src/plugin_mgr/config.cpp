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

bool check_plugin_list(YAML::Node plugin_list_item) {
    if (plugin_list_item["name"].IsNull()) {
        std::cerr << "WARN: null name in plugin_list.\n";
        return false;
    }
    if (plugin_list_item["url"].IsNull()) {
        std::cerr << "WARN: null url in plugin_list.\n";
        return false;
    }
    return true;
}

bool Config::load(const std::string &path) {
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
                for (size_t i = 0; i < plugin_list.size(); ++i) {
                    if (!check_plugin_list(plugin_list[i])){
                        continue;
                    }
                    std::string name = plugin_list[i]["name"].as<std::string>();
                    std::string description = plugin_list[i]["description"].as<std::string>();
                    std::string url = plugin_list[i]["url"].as<std::string>();
                    PluginInfo info(name, description, url);
                    if (this->plugin_list.count(name)) {
                        std::cerr << "Warn: duplicate \"" << name << "\" in plugin_list.\n";
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
                for (size_t i = 0; i < enable_list.size(); ++i) {
                    std::string name = enable_list[i]["name"].as<std::string>();
                    YAML::Node instances = enable_list[i]["instances"];
                    EnableItem enable_item(name);
                    if (!instances.IsDefined() || instances.IsNull()) {
                        enable_item.set_enabled(true);
                    } else {
                        for (size_t j = 0; j < instances.size(); ++j) {
                            std::string i_name = instances[j].as<std::string>();
                            enable_item.add_instance(i_name);
                        }
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

std::string get_path() {
    return DEFAULT_PLUGIN_PATH;
}
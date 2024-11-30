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
#include <iostream>
#include <unistd.h>
#include "default_path.h"

namespace oeaware {
bool CheckPluginList(YAML::Node pluginListItem)
{
    if (pluginListItem["name"].IsNull()) {
        std::cerr << "WARN: null name in plugin_list.\n";
        return false;
    }
    if (pluginListItem["url"].IsNull()) {
        std::cerr << "WARN: null url in plugin_list.\n";
        return false;
    }
    return true;
}

void Config::SetPluginList(const YAML::Node &node)
{
    YAML::Node pluginList = node["plugin_list"];
    if (!pluginList.IsSequence()) {
        std::cerr << "Error: 'plugin_list' is not a sequence" << '\n';
        return;
    }
    for (size_t i = 0; i < pluginList.size(); ++i) {
        if (!CheckPluginList(pluginList[i])) {
            continue;
        }
        std::string name = pluginList[i]["name"].as<std::string>();
        std::string description = pluginList[i]["description"].as<std::string>();
        std::string url = pluginList[i]["url"].as<std::string>();
        PluginInfo info(name, description, url);
        if (this->pluginList.count(name)) {
            std::cerr << "Warn: duplicate \"" << name << "\" in plugin_list.\n";
            continue;
        }
        this->pluginList.insert(std::make_pair(name, info));
    }
}

void Config::SetEnableList(const YAML::Node &node)
{
    YAML::Node enableList = node["enable_list"];
    if (!enableList.IsSequence()) {
        return;
    }
    for (size_t i = 0; i < enableList.size(); ++i) {
        std::string pluginName = enableList[i]["name"].as<std::string>();
        YAML::Node instances = enableList[i]["instances"];
        EnableItem enableItem(pluginName);
        if (!instances.IsSequence()) {
            WARN(logger, "the format of the enable list is incorrect.");
            continue;
        }
        if (!instances.IsDefined() || instances.IsNull()) {
            enableItem.SetEnabled(true);
        } else {
            for (size_t j = 0; j < instances.size(); ++j) {
                std::string instanceName = instances[j].as<std::string>();
                enableItem.AddInstance(instanceName);
            }
        }
        this->enableList.emplace_back(enableItem);
    }
}

bool Config::Load(const std::string &path)
{
    logger = Logger::GetInstance().Get("Main");
    YAML::Node node;
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return false;
    }
    try {
        node = YAML::LoadFile(path);
        if (!node.IsMap()) {
            return false;
        }
        if (!node["log_path"].IsNull()) {
            this->logPath = node["log_path"].as<std::string>();
        }
        if (!node["log_level"].IsNull()) {
            this->logLevel = node["log_level"].as<int>();
        }
        if (!node["plugin_list"].IsNull()) {
            SetPluginList(node);
        }
        if (!node["enable_list"].IsNull()) {
            SetEnableList(node);
        }
    }
    catch (const YAML::Exception& e) {
        std::cerr << e.what() << '\n';
        return false;
    }
    CreateDir(this->logPath);
    return true;
}

std::string GetPath()
{
    return DEFAULT_PLUGIN_PATH;
}
}

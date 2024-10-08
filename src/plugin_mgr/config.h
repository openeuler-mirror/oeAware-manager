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
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include <log4cplus/log4cplus.h>
#include <sys/stat.h>
#include "plugin.h"

namespace oeaware {
class PluginInfo {
public:
    PluginInfo(const std::string &name, const std::string &description, const std::string &url) : name(name), description(description), url(url) { }
    std::string GetName() const
    {
        return this->name;
    }
    std::string GetDescription() const
    {
        return this->description;
    }
    std::string GetUrl() const
    {
        return this->url;
    }
    bool operator ==(const PluginInfo &other) const
    {
        return name == other.GetName();
    }
private:
    std::string name;
    std::string description;
    std::string url;
};

class EnableItem {
public:
    explicit EnableItem(const std::string &name) : name(name), enabled(false) { }
    void SetEnabled(bool newEnabled)
    {
        this->enabled = newEnabled;
    }
    bool GetEnabled() const
    {
        return this->enabled;
    }
    void AddInstance(std::string text)
    {
        this->instance.emplace_back(text);
    }
    size_t GetInstanceSize() const
    {
        return this->instance.size();
    }
    std::string GetInstanceName(int i) const
    {
        return this->instance[i];
    }
    std::string GetName() const
    {
        return this->name;
    }
private:
    std::vector<std::string> instance;
    std::string name;
    bool enabled;
};

class Config {
public:
    Config()
    {
        logLevel = log4cplus::INFO_LOG_LEVEL;
    }
    bool Load(const std::string &path);
    int GetLogLevel() const
    {
        return this->logLevel;
    }
    std::string GetLogType() const
    {
        return this->logType;
    }
    std::string GetLogPath() const
    {
        return this->logPath;
    }
    PluginInfo GetPluginInfo(const std::string &name) const
    {
        return this->pluginList.at(name);
    }
    std::unordered_map<std::string, PluginInfo> GetPluginList() const
    {
        return this->pluginList;
    }
    size_t GetPluginListSize() const
    {
        return this->pluginList.size();
    }
    bool IsPluginInfoExist(const std::string &name)
    {
        return this->pluginList.count(name);
    }
    EnableItem GetEnableList(int i) const
    {
        return this->enableList[i];
    }
    size_t GetEnableListSize() const
    {
        return this->enableList.size();
    }
    void SetPluginList(const YAML::Node &node);
    void SetEnableList(const YAML::Node &node);
private:
    int logLevel;
    std::string logPath;
    std::string logType;
    std::unordered_map<std::string, PluginInfo> pluginList;
    std::vector<EnableItem> enableList;
};

std::string GetPath();
bool CreateDir(const std::string &path);
}

#endif

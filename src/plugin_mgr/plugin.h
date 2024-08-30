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
#ifndef PLUGIN_MGR_PLUGIN_H
#define PLUGIN_MGR_PLUGIN_H
#include <string>
#include <vector>
#include <memory>
#include <dlfcn.h>
#include "interface.h"

namespace oeaware {
class Instance {
public:
    void SetName(const std::string &name)
    {
        this->name = name;
    }
    std::string GetName() const
    {
        return this->name;
    }
    int GetPriority() const
    {
        return interface->get_priority();
    }
    Interface* GetInterface() const
    {
        return this->interface;
    }
    void SetPluginName(const std::string &name)
    {
        this->pluginName = name;
    }
    std::string GetPluginName() const
    {
        return this->pluginName;
    }
    void SetState(bool newState)
    {
        this->state = newState;
    }
    bool GetState() const
    {
        return this->state;
    }
    void SetEnabled(bool newEnabled)
    {
        this->enabled = newEnabled;
    }
    bool GetEnabled() const
    {
        return this->enabled;
    }
    void SetInterface(Interface *newInterface)
    {
        this->interface = newInterface;
    }
    std::string GetInfo() const;
    std::vector<std::string> GetDeps();
private:
    std::string name;
    std::string pluginName;
    bool state;
    bool enabled;
    Interface *interface;
    const static std::string pluginEnabled;
    const static std::string pluginDisabled;
    const static std::string pluginStateOn;
    const static std::string pluginStateOff;
};

class Plugin {
public:
    explicit Plugin(const std::string &name) : name(name), handler(nullptr) { }
    ~Plugin()
    {
        if (handler != nullptr) {
            dlclose(handler);
        }
    }
    int Load(const std::string &dl_path);
    std::string GetName() const
    {
        return this->name;
    }
    void AddInstance(std::shared_ptr<Instance> ins)
    {
        instances.emplace_back(ins);
    }
    std::shared_ptr<Instance> GetInstance(size_t i) const
    {
        return instances[i];
    }
    size_t GetInstanceLen() const
    {
        return instances.size();
    }
    void* GetHandler() const
    {
        return handler;
    }
private:
    std::vector<std::shared_ptr<Instance>> instances;
    std::string name;
    void *handler;
};
}

#endif

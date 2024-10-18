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
struct Instance {
    std::string name;
    std::string pluginName;
    bool state = true;
    bool enabled;
    std::shared_ptr<Interface> interface;
    std::unordered_map<std::string, Topic> supportTopics;
    const static std::string pluginEnabled;
    const static std::string pluginDisabled;
    const static std::string pluginStateOn;
    const static std::string pluginStateOff;
    std::string GetInfo() const;
};

class Plugin {
public:
    explicit Plugin(const std::string &name) : name(name), handler(nullptr) { }
    ~Plugin()
    {
        instances.clear();
        if (handler != nullptr) {
            dlclose(handler);
            handler = nullptr;
        }
    }
    int Load(const std::string &dl_path, std::shared_ptr<ManagerCallback> managerCallback);
    std::string GetName() const
    {
        return this->name;
    }
    std::shared_ptr<Instance> GetInstance(size_t i) const
    {
        return instances[i];
    }
    size_t GetInstanceLen() const
    {
        return instances.size();
    }
private:
    bool LoadInstance(std::shared_ptr<ManagerCallback> managerCallback);
    void SaveInstance(std::vector<std::shared_ptr<Interface>> &interfaceList,
        std::shared_ptr<ManagerCallback> managerCallback);
    int LoadDlInstance(std::vector<std::shared_ptr<Interface>> &interfaceList);
private:
    std::vector<std::shared_ptr<Instance>> instances;
    std::string name;
    void *handler;
};
}

#endif

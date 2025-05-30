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
#include <unordered_set>
#include "oeaware/interface.h"

namespace oeaware {
struct Instance {
    std::string name;
    std::string pluginName;
    bool state = true;
    bool enabled;
    uint64_t enableCnt = 0;
    std::unordered_map<std::string, Topic> supportTopics;
    std::shared_ptr<Interface> interface; // later move to private
    // value is topic type, used to close all topics when disable
    std::unordered_set<std::string> openTopics;
    const static std::string pluginEnabled;
    const static std::string pluginDisabled;
    const static std::string pluginStateOn;
    const static std::string pluginStateOff;
    std::string GetInfo() const;
    std::string GetName() const;
    std::string GetRun() const;
    Result OpenTopic(const Topic &topic);
    void CloseTopic(const Topic &topic);
    void Disable();
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
    int Load(const std::string &dl_path);
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
    std::shared_ptr<SafeQueue<std::shared_ptr<InstanceRunMessage>>> recvQueue;
private:
    bool LoadInstance();
    void SaveInstance(std::vector<std::shared_ptr<Interface>> &interfaceList);
    int LoadDlInstance(std::vector<std::shared_ptr<Interface>> &interfaceList);
private:
    std::vector<std::shared_ptr<Instance>> instances;
    std::string name;
    void *handler;
};
}

#endif

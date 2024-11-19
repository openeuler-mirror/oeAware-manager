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
#ifndef PLUGIN_MGR_MEMORY_STORE_H
#define PLUGIN_MGR_MEMORY_STORE_H
#include <unordered_map>
#include "logger.h"
#include "plugin.h"

namespace oeaware {
/* OeAware memory storage, which is used to store plugins and instances in the memory. */
class MemoryStore {
public:
    ~MemoryStore()
    {
        instances.clear();
        plugins.clear();
    }
    void AddPlugin(const std::string &name, std::shared_ptr<Plugin> plugin)
    {
        this->plugins.insert(std::make_pair(name, plugin));
    }
    void AddInstance(std::shared_ptr<Instance> instance)
    {
        instances.insert(std::make_pair(instance->name, instance));
    }
    std::shared_ptr<Plugin> GetPlugin(const std::string &name)
    {
        if (!plugins.count(name)) {
            return nullptr;
        }
        return plugins[name];
    }
    std::shared_ptr<Instance> GetInstance(const std::string &name)
    {
        if (!instances.count(name)) {
            return nullptr;
        }
        return instances[name];
    }
    void DeletePlugin(const std::string &name)
    {
        this->plugins.erase(name);
    }
    void DeleteInstance(const std::string &name)
    {
        instances.erase(name);
    }
    bool IsPluginExist(const std::string &name) const
    {
        return this->plugins.count(name);
    }
    bool IsInstanceExist(const std::string &name)
    {
        return instances.count(name);
    }
    const std::vector<std::shared_ptr<Plugin>> GetAllPlugins()
    {
        std::vector<std::shared_ptr<Plugin>> res;
        for (auto &p : plugins) {
            res.emplace_back(p.second);
        }
        return res;
    }
private:
    std::unordered_map<std::string, std::shared_ptr<Instance>> instances;
    std::unordered_map<std::string, std::shared_ptr<Plugin>> plugins;
};
}

#endif // !PLUGIN_MGR_MEMORY_STORE_H

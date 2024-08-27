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
#include "dep_handler.h"

namespace oeaware {
/* OeAware memory storage, which is used to store plugins and instances in the memory. */
class MemoryStore {
public:
    void AddPlugin(const std::string &name, std::shared_ptr<Plugin> plugin)
    {
        this->plugins.insert(std::make_pair(name, plugin));
    }
    void AddInstance(std::shared_ptr<Instance> instance)
    {
        depHandler.AddInstance(instance);
    }
    std::shared_ptr<Plugin> GetPlugin(const std::string &name) const
    {
        return this->plugins.at(name);
    }
    std::shared_ptr<Instance> GetInstance(const std::string &name) const
    {
        return depHandler.GetInstance(name);
    }
    void DeletePlugin(const std::string &name)
    {
        this->plugins.erase(name);
    }
    void DeleteInstance(const std::string &name)
    {
        depHandler.DeleteInstance(name);
    }
    bool IsPluginExist(const std::string &name) const
    {
        return this->plugins.count(name);
    }
    bool IsInstanceExist(const std::string &name)
    {
        return depHandler.IsInstanceExist(name);
    }
    const std::vector<std::shared_ptr<Plugin>> GetAllPlugins()
    {
        std::vector<std::shared_ptr<Plugin>> res;
        for (auto &p : plugins) {
            res.emplace_back(p.second);
        }
        return res;
    }
    void QueryNodeDependency(const std::string &name, std::vector<std::vector<std::string>> &query)
    {
        return depHandler.QueryNodeDependency(name, query);
    }
    void QueryAllDependencies(std::vector<std::vector<std::string>> &query)
    {
        return depHandler.QueryAllDependencies(query);
    }
    bool HaveDep(const std::string &name)
    {
        return depHandler.HaveDep(name);
    }
    const DepHandler& GetDepHandler() const
    {
        return depHandler;
    }
    void AddEdge(const std::string &from, const std::string &to)
    {
        depHandler.AddEdge(from, to);
    }
    void DeleteEdge(const std::string &from, const std::string &to)
    {
        depHandler.DeleteEdge(from, to);
    }
private:
    /* instance are stored in the form of DAG.
     * DepHandler stores instances and manages dependencies.
     */
    DepHandler depHandler;
    std::unordered_map<std::string, std::shared_ptr<Plugin>> plugins;
};
}

#endif // !PLUGIN_MGR_MEMORY_STORE_H

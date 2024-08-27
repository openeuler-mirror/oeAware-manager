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
#ifndef PLUGIN_MGR_PLUGIN_MANAGER_H
#define PLUGIN_MGR_PLUGIN_MANAGER_H
#include "instance_run_handler.h"
#include "config.h"
#include "memory_store.h"
#include "message_manager.h"
#include "error_code.h"

namespace oeaware {
class PluginManager {
public:
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    static PluginManager& GetInstance()
    {
        static PluginManager pluginManager;
        return pluginManager;
    }
    int Run();
    void Init(std::shared_ptr<Config> config, std::shared_ptr<SafeQueue<Message>> handlerMsg,
    std::shared_ptr<SafeQueue<Message>> resMsg);
    const MemoryStore& GeMemoryStore()
    {
        return this->memoryStore;
    }
    void Exit();
    void SendMsg(const Message &msg)
    {
        handlerMsg->Push(msg);
    }
private:
    PluginManager() { }
    void PreLoad();
    void PreEnable();
    void PreLoadPlugin();
    ErrorCode LoadPlugin(const std::string &path);
    ErrorCode Remove(const std::string &name);
    ErrorCode QueryAllPlugins(std::string &res);
    ErrorCode QueryPlugin(const std::string &name, std::string &res);
    ErrorCode QueryDependency(const std::string &name, std::string &res);
    ErrorCode QueryAllDependencies(std::string &res);
    ErrorCode InstanceEnabled(const std::string &name);
    ErrorCode InstanceDisabled(const std::string &name);
    ErrorCode AddList(std::string &res);
    ErrorCode Download(const std::string &name, std::string &res);
    std::string InstanceDepCheck(const std::string &name);
    int LoadDlInstance(std::shared_ptr<Plugin> plugin, Interface **interfaceList);
    void SaveInstance(std::shared_ptr<Plugin> plugin, Interface *interfaceList, int len);
    bool LoadInstance(std::shared_ptr<Plugin> plugin);
    void UpdateInstanceState();
    bool EndWith(const std::string &s, const std::string &ending);
    std::string GetPluginInDir(const std::string &path);
    void SendMsgToInstancRunHandler(std::shared_ptr<InstanceRunMessage> msg)
    {
        instanceRunHandler->RecvQueuePush(msg);
    }
    void EnablePlugin(const std::string &name);
    void EnableInstance(const EnableItem &item);
    void ConstructLackDep(const std::vector<std::vector<std::string>> &query, std::string &res);
private:
    std::unique_ptr<InstanceRunHandler> instanceRunHandler;
    std::shared_ptr<Config> config;
    std::shared_ptr<SafeQueue<Message>> handlerMsg;
    std::shared_ptr<SafeQueue<Message>> resMsg;
    MemoryStore memoryStore;
};

void PrintHelp();
void SignalHandler(int signum);

}

#endif

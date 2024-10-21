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
#include "message_manager.h"
#include "event/event_handler.h"
#include "manager_callback.h"

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
    void Run();
    void Init(std::shared_ptr<Config> config, EventQueue recvMessage, EventResultQueue sendMessage,
        EventQueue recvData);
    void Exit();
    void SendMsg(const Event &msg)
    {
        recvMessage->Push(msg);
    }
private:
    PluginManager() { }
    void InitEventHandler();
    void PreLoad();
    void PreEnable();
    void PreLoadPlugin();
    void UpdateInstanceState();
    std::string GetPluginInDir(const std::string &path);
    void SendMsgToInstancRunHandler(std::shared_ptr<InstanceRunMessage> msg)
    {
        instanceRunHandler->RecvQueuePush(msg);
    }
    void EnablePlugin(const std::string &name);
    void EnableInstance(const EnableItem &item);
private:
    std::shared_ptr<InstanceRunHandler> instanceRunHandler;
    std::shared_ptr<Config> config;
    std::shared_ptr<SafeQueue<Event>> recvMessage;
    std::shared_ptr<SafeQueue<EventResult>> sendMessage;
    std::unordered_map<Opt, std::shared_ptr<Handler>> eventHandler;
    std::shared_ptr<ManagerCallback> managerCallback;
    std::shared_ptr<MemoryStore> memoryStore;
    log4cplus::Logger logger;
};

void PrintHelp();
void SignalHandler(int signum);

}

#endif

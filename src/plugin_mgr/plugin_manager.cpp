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
#include "plugin_manager.h"
#include <csignal>
#include <dirent.h>
#include "utils.h"
#include "event/load_handler.h"
#include "event/remove_handler.h"
#include "event/query_handler.h"
#include "event/query_dep_handler.h"
#include "event/enable_handler.h"
#include "event/disable_handler.h"
#include "event/list_handler.h"
#include "event/download_handler.h"

namespace oeaware {
void PrintHelp()
{
    printf("Usage: ./oeaware [path]\n"
            "      ./oeaware --help\n"
           "Examples:\n"
           "    ./oeaware /etc/oeAware/config.yaml\n");
}

void SignalHandler(int signum)
{
    (void)signum;
    oeaware::PluginManager::GetInstance().Exit();
    oeaware::MessageManager::GetInstance().Exit();
}

std::shared_ptr<MemoryStore> Handler::memoryStore;
log4cplus::Logger Handler::logger;

void PluginManager::InitEventHandler()
{
    Handler::memoryStore = memoryStore;
    Handler::logger = logger;
    eventHandler[Opt::LOAD] = std::make_shared<LoadHandler>();
    eventHandler[Opt::REMOVE] = std::make_shared<RemoveHandler>();
    eventHandler[Opt::QUERY_ALL] = std::make_shared<QueryHandler>();
    eventHandler[Opt::QUERY] = eventHandler[Opt::QUERY_ALL];
    eventHandler[Opt::QUERY_ALL_DEPS] = std::make_shared<QueryDepHandler>();
    eventHandler[Opt::QUERY_DEP] = eventHandler[Opt::QUERY_ALL_DEPS];
    eventHandler[Opt::ENABLED] = std::make_shared<EnableHandler>(instanceRunHandler);
    eventHandler[Opt::DISABLED] = std::make_shared<DisableHandler>(instanceRunHandler);
    eventHandler[Opt::LIST] = std::make_shared<ListHandler>(config);
    eventHandler[Opt::DOWNLOAD] = std::make_shared<DownloadHandler>(config);
}

void PluginManager::Init(std::shared_ptr<Config> config, std::shared_ptr<SafeQueue<Event>> recvMessage,
    std::shared_ptr<SafeQueue<EventResult>> sendMessage)
{
    this->config = config;
    this->recvMessage = recvMessage;
    this->sendMessage = sendMessage;
    memoryStore = std::make_shared<MemoryStore>();
    instanceRunHandler = std::make_shared<InstanceRunHandler>(memoryStore);
    Logger::GetInstance().Register("PluginManager");
    logger = Logger::GetInstance().Get("PluginManager");
    InitEventHandler();
}

void PluginManager::EnablePlugin(const std::string &pluginName)
{
    std::shared_ptr<Plugin> plugin = memoryStore->GetPlugin(pluginName);
    for (size_t j = 0; j < plugin->GetInstanceLen(); ++j) {
        std::string name = plugin->GetInstance(j)->GetName();
        SendMsg(Event(Opt::ENABLED, EventType::INTERNAL, {name}));
    }
}

void PluginManager::EnableInstance(const EnableItem &item)
{
    for (size_t j = 0; j < item.GetInstanceSize(); ++j) {
        std::string name = item.GetInstanceName(j);
        SendMsg(Event(Opt::ENABLED, EventType::INTERNAL, {name}));
    }
}

void PluginManager::PreEnable()
{
    for (size_t i = 0; i < config->GetEnableListSize(); ++i) {
        EnableItem item = config->GetEnableList(i);
        std::string pluginName = item.GetName();
        if (!memoryStore->IsPluginExist(pluginName)) {
            WARN(logger, "plugin " << pluginName << " cannot be enabled, because it does not exist.");
            continue;
        }
        if (item.GetEnabled()) {
            EnablePlugin(pluginName);
        } else {
            EnableInstance(item);
        }
    }
}

void PluginManager::PreLoadPlugin()
{
    std::string path = GetPath();
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (EndWith(name, ".so")) {
            eventHandler[Opt::LOAD]->Handle(Event(Opt::LOAD, EventType::INTERNAL, {name}));
        }
    }
    closedir(dir);
}

void PluginManager::PreLoad()
{
    PreLoadPlugin();
    PreEnable();
}

void PluginManager::Exit()
{
    auto allPlugins = memoryStore->GetAllPlugins();
    auto msg = std::make_shared<InstanceRunMessage>(RunType::SHUTDOWN, nullptr);
    SendMsgToInstancRunHandler(msg);
    msg->Wait();
    for (auto plugin : allPlugins) {
        for (size_t i = 0; i < plugin->GetInstanceLen(); ++i) {
            auto instance = plugin->GetInstance(i);
            if (!instance->GetEnabled()) {
                continue;
            }
            instance->GetInterface()->disable();
            INFO(logger, instance->GetName() << " instance disabled.");
        }
    }
    this->recvMessage->Push(Event(Opt::SHUTDOWN));
}

int PluginManager::Run()
{
    instanceRunHandler->Run();
    PreLoad();
    while (true) {
        Event msg;
        this->recvMessage->WaitAndPop(msg);
        if (msg.GetOpt() == Opt::SHUTDOWN) {
            break;
        }
        auto handler = eventHandler[msg.GetOpt()];
        auto eventResult = handler->Handle(msg);
        if (msg.GetType() == EventType::EXTERNAL) {
            sendMessage->Push(eventResult);
        }
    }
    return 0;
}
}

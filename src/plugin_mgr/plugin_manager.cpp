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
#include "event/enable_handler.h"
#include "event/disable_handler.h"
#include "event/list_handler.h"
#include "event/download_handler.h"
#include "event/subscribe_handler.h"
#include "event/unsubscribe_handler.h"
#include "event/publish_handler.h"

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
    oeaware::PluginManager::GetInstance().SendMsg(Event(Opt::SHUTDOWN));
}

std::shared_ptr<MemoryStore> Handler::memoryStore;
log4cplus::Logger Handler::logger;

void PluginManager::InitEventHandler()
{
    Handler::memoryStore = memoryStore;
    Handler::logger = logger;
    eventHandler[Opt::LOAD] = std::make_shared<LoadHandler>(managerCallback);
    eventHandler[Opt::REMOVE] = std::make_shared<RemoveHandler>();
    eventHandler[Opt::QUERY_ALL] = std::make_shared<QueryHandler>();
    eventHandler[Opt::QUERY] = eventHandler[Opt::QUERY_ALL];
    eventHandler[Opt::ENABLED] = std::make_shared<EnableHandler>(instanceRunHandler);
    eventHandler[Opt::DISABLED] = std::make_shared<DisableHandler>(instanceRunHandler);
    eventHandler[Opt::LIST] = std::make_shared<ListHandler>(config);
    eventHandler[Opt::DOWNLOAD] = std::make_shared<DownloadHandler>(config);
    eventHandler[Opt::SUBSCRIBE] = std::make_shared<SubscribeHandler>(managerCallback, instanceRunHandler);
    eventHandler[Opt::UNSUBSCRIBE] = std::make_shared<UnsubscribeHandler>(managerCallback, instanceRunHandler);
    eventHandler[Opt::PUBLISH] = std::make_shared<PublishHandler>();
}

void PluginManager::Init(std::shared_ptr<Config> config, EventQueue recvMessage, EventResultQueue sendMessage,
    EventQueue recvData)
{
    this->config = config;
    this->recvMessage = recvMessage;
    this->sendMessage = sendMessage;
    managerCallback = std::make_shared<ManagerCallback>();
    managerCallback->Init(recvData);
    memoryStore = std::make_shared<MemoryStore>();
    instanceRunHandler = std::make_shared<InstanceRunHandler>(memoryStore, managerCallback);
    Logger::GetInstance().Register("PluginManager");
    Logger::GetInstance().Register("Plugin");
    logger = Logger::GetInstance().Get("PluginManager");
    InitEventHandler();
}

void PluginManager::EnablePlugin(const std::string &pluginName)
{
    std::shared_ptr<Plugin> plugin = memoryStore->GetPlugin(pluginName);
    for (size_t j = 0; j < plugin->GetInstanceLen(); ++j) {
        std::string name = plugin->GetInstance(j)->name;
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
    auto msg = std::make_shared<InstanceRunMessage>(RunType::SHUTDOWN);
    SendMsgToInstancRunHandler(msg);
    msg->Wait();
    for (auto plugin : allPlugins) {
        for (size_t i = 0; i < plugin->GetInstanceLen(); ++i) {
            auto instance = plugin->GetInstance(i);
            if (!instance->enabled) {
                continue;
            }
            for (auto &p : instance->supportTopics) {
                instance->interface->CloseTopic(p.second);
            }
            instance->interface->Disable();
            INFO(logger, instance->name << " instance disabled.");
        }
    }
    INFO(logger, "oeaware shutdown.");
}

void PluginManager::Run()
{
    instanceRunHandler->Run();
    PreLoad();
    while (true) {
        Event event;
        this->recvMessage->WaitAndPop(event);
        if (event.opt == Opt::SHUTDOWN) {
            break;
        }
        auto handler = eventHandler[event.opt];
        auto eventResult = handler->Handle(event);
        if (event.type == EventType::EXTERNAL) {
            sendMessage->Push(eventResult);
        }
    }
}
}

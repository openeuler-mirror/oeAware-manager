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
#include <csignal>
#include "plugin_manager.h"
#include "message_manager.h"
#include "oeaware/utils.h"

int main(int argc, char **argv)
{
    oeaware::CreateDir(oeaware::DEFAULT_LOG_PATH);
    oeaware::Logger::GetInstance().Register("Main");
    auto logger = oeaware::Logger::GetInstance().Get("Main");
    if (signal(SIGINT, oeaware::SignalHandler) == SIG_ERR || signal(SIGTERM, oeaware::SignalHandler) == SIG_ERR) {
        ERROR(logger, "Sig Error!");
        exit(EXIT_FAILURE);
    }
    std::shared_ptr<oeaware::Config> config = std::make_shared<oeaware::Config>();
    int argCnt = 2;
    if (argc < argCnt) {
        ERROR(logger, "System need an argument!");
        oeaware::PrintHelp();
        exit(EXIT_FAILURE);
    }
    if (std::string(argv[1]) == "--help") {
        oeaware::PrintHelp();
        exit(EXIT_SUCCESS);
    }
    if (!oeaware::FileExist(argv[1])) {
        ERROR(logger, "Config file " << argv[1] << " does not exist!");
        exit(EXIT_FAILURE);
    }
    std::string config_path(argv[1]);
    if (!oeaware::CheckPermission(config_path, S_IRUSR | S_IWUSR | S_IRGRP)) {
        ERROR(logger, "Insufficient permission on " << config_path);
        exit(EXIT_FAILURE);
    }
    if (!config->Load(config_path)) {
        ERROR(logger, "Config load error!");
        exit(EXIT_FAILURE);
    }
    if (oeaware::Logger::GetInstance().Init(config->GetLogPath(), config->GetLogLevel()) < 0) {
        ERROR(logger, "logger init failed!");
        exit(EXIT_FAILURE);
    }
    INFO(logger, "log path: " << config->GetLogPath() << ", log level: " << config->GetLogLevel());
    auto recvMessage = std::make_shared<oeaware::SafeQueue<oeaware::Event>>();
    auto sendMessage = std::make_shared<oeaware::SafeQueue<oeaware::EventResult>>();
    auto recvData = std::make_shared<oeaware::SafeQueue<oeaware::Event>>();
    INFO(logger, "Start message manager!");
    oeaware::MessageManager &messageManager = oeaware::MessageManager::GetInstance();
    if (!messageManager.Init(recvMessage, sendMessage, recvData)) {
        ERROR(logger, "MessageManager init failed!");
        exit(EXIT_FAILURE);
    }
    messageManager.Run();
    oeaware::Register::GetInstance().InitRegisterData();
    INFO(logger, "Start plugin manager!");
    oeaware::PluginManager& pluginManager = oeaware::PluginManager::GetInstance();
    pluginManager.Init(config, recvMessage, sendMessage, recvData);
    pluginManager.Run();
    pluginManager.Exit();
    messageManager.Exit();
    return 0;
}

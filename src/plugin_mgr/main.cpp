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
#include "utils.h"

oeaware::Logger g_logger;

int main(int argc, char **argv) {
    if (signal(SIGINT, oeaware::SignalHandler) == SIG_ERR || signal(SIGTERM, oeaware::SignalHandler) == SIG_ERR ||
        signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        ERROR("Sig Error!");
        exit(EXIT_FAILURE);
    }
    std::shared_ptr<oeaware::Config> config = std::make_shared<oeaware::Config>();
    int argCnt = 2;
    if (argc < argCnt) {
        ERROR("System need an argument!");
        oeaware::PrintHelp();
        exit(EXIT_FAILURE);
    }
    if (std::string(argv[1]) == "--help") {
        oeaware::PrintHelp();
        exit(EXIT_SUCCESS);
    }
    if (!oeaware::FileExist(argv[1])) {
        ERROR("Config file " << argv[1] << " does not exist!");
        exit(EXIT_FAILURE);
    }
    std::string config_path(argv[1]);
    if (!oeaware::CheckPermission(config_path, S_IRUSR | S_IWUSR | S_IRGRP)) {
        ERROR("Insufficient permission on " << config_path);
        exit(EXIT_FAILURE);
    }
    if (!config->Load(config_path)) {
        ERROR("Config load error!");
        exit(EXIT_FAILURE);
    }
    g_logger.Init(config->GetLogPath(), config->GetLogLevel());
    auto recvMessage = std::make_shared<oeaware::SafeQueue<oeaware::Event>>();
    auto sendMessage = std::make_shared<oeaware::SafeQueue<oeaware::EventResult>>();
    INFO("[MessageManager] Start message manager!");
    oeaware::MessageManager &messageManager = oeaware::MessageManager::GetInstance();
    messageManager.Init(recvMessage, sendMessage);
    messageManager.Run();
    INFO("[PluginManager] Start plugin manager!");
    oeaware::PluginManager& pluginManager = oeaware::PluginManager::GetInstance();
    pluginManager.Init(config, recvMessage, sendMessage);
    pluginManager.Run();
    return 0;
}

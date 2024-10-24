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

Logger logger;

void print_help() {
    printf("Usage: ./oeaware [path]\n"
            "      ./oeaware --help\n"
           "Examples:\n"
           "    ./oeaware /etc/oeAware/config.yaml\n");
}

void signal_handler(int signum) {
    if (signum != SIGINT && signum != SIGTERM) {
        ERROR("Unknown signal: " << signum);
        exit(signum);
    }
    auto &plugin_manager = PluginManager::get_instance();
    plugin_manager.send_msg(Message(Opt::SHUTDOWN, MessageType::INTERNAL));
}

int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);  // ctrl + c
    signal(SIGTERM, signal_handler);  // systemctl stop
    std::shared_ptr<Config> config = std::make_shared<Config>();
    if (argc < 2) {
        ERROR("System need an argument!");
        print_help();
        exit(EXIT_FAILURE);
    }
    if (std::string(argv[1]) == "--help") {
        print_help();
        exit(EXIT_SUCCESS);
    }
    if (!file_exist(argv[1])) {
        ERROR("Config file " << argv[1] << " does not exist!");
        exit(EXIT_FAILURE);
    }
    std::string config_path(argv[1]);
    if (!check_permission(config_path, S_IRUSR | S_IWUSR | S_IRGRP)) {
        ERROR("Insufficient permission on " << config_path);
        exit(EXIT_FAILURE);
    }
    if (!config->load(config_path)) {
        ERROR("Config load error!");
        exit(EXIT_FAILURE);
    }
    logger.init(config);
    std::shared_ptr<SafeQueue<Message>> handler_msg = std::make_shared<SafeQueue<Message>>();
    std::shared_ptr<SafeQueue<Message>> res_msg = std::make_shared<SafeQueue<Message>>();
    INFO("[MessageManager] Start message manager!");
    MessageManager &message_manager = MessageManager::get_instance();
    message_manager.init(handler_msg, res_msg);
    message_manager.run();
    INFO("[PluginManager] Start plugin manager!");
    PluginManager& plugin_manager = PluginManager::get_instance();
    plugin_manager.init(config, handler_msg, res_msg);
    plugin_manager.run();
    return 0;
}
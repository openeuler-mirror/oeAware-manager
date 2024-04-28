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

Logger logger;

void print_help() {
    printf("Usage: ./oeaware [path]\n"
            "      ./oeaware --help\n"
           "Examples:\n"
           "    ./oeaware /etc/oeAware/config.yaml\n");
}

int main(int argc, char **argv) {
    Config config;
    if (argc < 2) {
        ERROR("System need a argument!");
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
    if (!config.load(config_path)) {
        ERROR("Config load error!");
        exit(EXIT_FAILURE);
    }
    logger.init(&config);
    SafeQueue<Message> handler_msg;
    SafeQueue<Message> res_msg;
    INFO("[MessageManager] Start message manager!");
    MessageManager message_manager(&handler_msg, &res_msg);
    message_manager.init(&config);
    message_manager.run();
    INFO("[PluginManager] Start plugin manager!");
    PluginManager plugin_manager(config, handler_msg, res_msg);
    plugin_manager.init();
    plugin_manager.pre_load();
    plugin_manager.run();
    return 0;
}
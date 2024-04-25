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
#ifndef CLIENT_H
#define CLIENT_H
#include "tcp_socket.h"
#include "cmd_handler.h"
#include <getopt.h>

class Client {
public:
    Client() : cmd_handler(nullptr) { }
    ~Client() {
        if (cmd_handler)
            delete cmd_handler;
    }
    bool init();
    void run_cmd(int cmd);
    void close();
    int arg_parse(int argc, char *argv[]);
private:
    void arg_error(const std::string &msg);
    
    TcpSocket tcp_socket;
    CmdHandler *cmd_handler;
    static const std::string OPT_STRING;
    static const int MAX_OPT_SIZE = 20;
    static const struct option long_options[MAX_OPT_SIZE];
};

#endif // !CLIENT_H

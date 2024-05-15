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
#include <unordered_map>
#include <memory>

class Client {
public:
    Client() : cmd_handler(nullptr) { }
    bool init(int argc, char *argv[]);
    void run_cmd();
    void close();
private:
    void cmd_groups_init();
    
    int cmd;
    TcpSocket tcp_socket;
    std::shared_ptr<CmdHandler> cmd_handler;
    std::unordered_map<int, std::shared_ptr<CmdHandler>> cmd_handler_groups;
};

#endif // !CLIENT_H

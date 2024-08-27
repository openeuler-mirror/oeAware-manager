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
#include <unordered_map>
#include <memory>
#include "tcp_socket.h"
#include "cmd_handler.h"

namespace oeaware {
class Client {
public:
    Client() : cmdHandler(nullptr) { }
    bool Init(int argc, char *argv[]);
    void RunCmd();
    void Close();
private:
    void CmdGroupsInit();
    
    int cmd;
    TcpSocket tcpSocket;
    std::shared_ptr<CmdHandler> cmdHandler;
    std::unordered_map<int, std::shared_ptr<CmdHandler>> cmdHandlerGroups;
};
}

#endif // !CLIENT_H

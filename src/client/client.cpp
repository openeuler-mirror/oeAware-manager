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
#include "client.h"

std::string ArgParse::arg;
std::string ArgParse::type;
std::unordered_set<char> ArgParse::opts;

void Client::cmd_groups_init() {
    cmd_handler_groups.insert(std::make_pair('l', std::make_shared<LoadHandler>()));
    cmd_handler_groups.insert(std::make_pair('q', std::make_shared<QueryHandler>()));
    cmd_handler_groups.insert(std::make_pair('r', std::make_shared<RemoveHandler>()));
    cmd_handler_groups.insert(std::make_pair('Q', std::make_shared<QueryTopHandler>()));
    cmd_handler_groups.insert(std::make_pair('e', std::make_shared<EnabledHandler>()));
    cmd_handler_groups.insert(std::make_pair('d', std::make_shared<DisabledHandler>()));
    cmd_handler_groups.insert(std::make_pair('L', std::make_shared<ListHandler>()));
    cmd_handler_groups.insert(std::make_pair('i', std::make_shared<InstallHandler>()));
}

bool Client::init(int argc, char *argv[]) {
    if ((cmd = ArgParse::init(argc, argv)) < 0) {
        return false;
    }
    cmd_groups_init();
    return this->tcp_socket.init();
}

void Client::run_cmd() {
    Msg msg;
    Msg res;
    MessageHeader header;
    this->cmd_handler = cmd_handler_groups[cmd];
    this->cmd_handler->handler(msg);
    if(!this->tcp_socket.send_msg(msg, header)) {
        return;
    }
    if (!this->tcp_socket.recv_msg(res, header)) {
        return;
    }
    this->cmd_handler->res_handler(res);
}

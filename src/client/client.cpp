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

void Client::cmd_groups_init() {
    cmd_handler_groups.insert(std::make_pair('l', new LoadHandler()));
    cmd_handler_groups.insert(std::make_pair('q', new QueryHandler()));
    cmd_handler_groups.insert(std::make_pair('r', new RemoveHandler()));
    cmd_handler_groups.insert(std::make_pair('Q', new QueryTopHandler()));
    cmd_handler_groups.insert(std::make_pair('e', new EnabledHandler()));
    cmd_handler_groups.insert(std::make_pair('d', new DisabledHandler()));
    cmd_handler_groups.insert(std::make_pair('L', new ListHandler()));
    cmd_handler_groups.insert(std::make_pair('i', new InstallHandler(arg_parse.get_arg())));
}
bool Client::init(int argc, char *argv[]) {
    if ((cmd = arg_parse.init(argc, argv)) < 0) {
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
    this->cmd_handler->handler(arg_parse, msg);
    if(!this->tcp_socket.send_msg(msg)) {
        return;
    }
    if (!this->tcp_socket.recv_msg(res, header)) {
        return;
    }
    this->cmd_handler->res_handler(res);
}

void Client::close() {
    tcp_socket.clear();
}

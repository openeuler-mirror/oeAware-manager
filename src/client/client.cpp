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

static void print_error(const Msg &msg) {
    for (int i = 0; i < msg.payload_size(); ++i) {
        printf("%s\n", msg.payload(i).c_str());
    }
}

bool Client::init() {
    return this->tcp_socket.init();
}

void Client::run_cmd(int cmd) {
    Msg msg;
    Msg res;
    MessageHeader header;
    this->cmd_handler = get_cmd_handler(cmd);
    this->cmd_handler->handler(msg);
    if(!this->tcp_socket.send_msg(msg)) {
        return;
    }
    if (!this->tcp_socket.recv_msg(res, header)) {
        return;
    }
    if (header.get_state_code() == HEADER_STATE_FAILED) {
        print_error(res);
        return;
    }
    this->cmd_handler->res_handler(res);
}

const std::string Client::OPT_STRING = "Qqd:t:l:r:e:";
const struct option Client::long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"load", required_argument, NULL, 'l'},
    {"type", required_argument, NULL, 't'},
    {"remove", required_argument, NULL, 'r'}, 
    {"query", required_argument, NULL, 'q'},
    {"query-dep", required_argument, NULL, 'Q'},
    {"enable", required_argument, NULL, 'e'},
    {"disable", required_argument, NULL, 'd'},
    {"list", no_argument, NULL, 'L'},
    {"download", required_argument, NULL, 'D'},
    {0, 0, 0, 0},
};
int Client::arg_parse(int argc, char *argv[]) {
    int cmd = -1;
    int opt;
    bool help = false;
    opterr = 0;
    while((opt = getopt_long(argc, argv, OPT_STRING.c_str(), long_options, nullptr)) != -1) {
        std::string full_opt;
        switch (opt) {
            case 't':
                set_type(optarg);
                break;
            case 'h':
                help = true;
                break;
            case '?':
                arg_error("unknown option.  See --help.");
                return -1;
            default: {
                if (opt == 'l' || opt == 'r' || opt == 'q' || opt == 'Q' || opt == 'e' || opt == 'd'  || opt == 'L' || opt == 'D') {
                    if (cmd != -1) {
                        arg_error("invalid option. See --help.\n");
                        return -1;
                    }
                    cmd = opt;
                    if (optarg) {
                        set_arg(optarg);
                    }
                } 
            }
                
        }
    }
    if (help) {
        print_help();
        exit(EXIT_SUCCESS);
    }
    if (cmd < 0) {
        arg_error("no option.");
    }
    return cmd;
}
void Client::arg_error(const std::string &msg) {
    std::cerr << "oeawarectl: " << msg << "\n";
    exit(EXIT_FAILURE);
}
void Client::close() {
    tcp_socket.clear();
}
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
#include <vector>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include "config.h"
#include "message.h"
#include "message_protocol.h"
#include "cmd_handler.h"

const std::string OPT_STRING = "Qqd:t:l:r:e:";

class TcpSocket {
public:
    TcpSocket() { }
    bool recv_msg(server::Msg *res) {
        char buf[MAX_BUFF_SIZE];
        int ret = recv(sock, buf, sizeof(buf), 0);
        if (ret < 0) {
            printf("can't connect to server!\n");
            return false;
        }
        decode(buf, res);
        return true;
    }

    bool send_msg(server::Msg *msg) {
        int len = 0;
        char buf[MAX_BUFF_SIZE];
        encode(buf, len, msg);
        if (send(sock, buf, len, 0) < 0) {
            return false;
        }
        return true;
    }
    int file_connect(const char *name) {
        struct sockaddr_un un;
        int len;
        int fd;
        if( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            printf("socket create error!\n");
            return -1;
        }
        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, name);
        len = offsetof(struct sockaddr_un, sun_path) + strlen(name);
        if (connect(fd, (struct sockaddr*)&un, len) < 0) {
            printf("can't connect to server!\n");
            return -1;
        }
        return fd;
    }

    bool init() {
        std::string path = DEFAULT_RUN_PATH + "/oeAware-sock";
        if ((sock = file_connect(path.c_str())) < 0) {
            return false;
        }
        return true;
    }
    void clear() {
        close(sock);
    }
private:
    int sock;
};

class Client {
public:
    Client() : cmd_handler(nullptr) {}
    ~Client() {
        if (cmd_handler)
            delete cmd_handler;
    }
    bool init() {
        return this->tcp_socket.init();
    }

    bool run_cmd(int cmd) {
        server::Msg msg;
        server::Msg res;
        this->cmd_handler = get_cmd_handler(cmd);
        this->cmd_handler->handler(&msg);
        this->tcp_socket.send_msg(&msg);
        if (!this->tcp_socket.recv_msg(&res)) return false;
        return this->cmd_handler->res_handler(&res);
    }
    void close() {
        tcp_socket.clear();
    }
private:
    TcpSocket tcp_socket;
    CmdHandler *cmd_handler;
};

static struct option long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"load", required_argument, NULL, 'l'},
    {"type", required_argument, NULL, 't'},
    {"remove", required_argument, NULL, 'r'}, 
    {"query", required_argument, NULL, 'q'},
    {"query-top", required_argument, NULL, 'Q'},
    {"enable", required_argument, NULL, 'e'},
    {"disable", required_argument, NULL, 'd'},
    {"list", no_argument, NULL, 'L'},
    {"download", required_argument, NULL, 'D'},
    {0, 0, 0, 0},
};


int arg_parse(int argc, char *argv[]) {
    int cmd = -1;
    int opt;
    bool help = false;
    while((opt = getopt_long(argc, argv, OPT_STRING.c_str(), long_options, nullptr)) != -1) {
        switch (opt) {
            case 't':
                set_type(optarg);
                break;
            case 'h':
                help = true;
                break;
            case '?':
                return -1;
            default:  
                if (opt == 'l' || opt == 'r' || opt == 'q' || opt == 'Q' || opt == 'e' || opt == 'd'  || opt == 'L') {
                    if (cmd != -1) {
                        printf("error: invalid option. See --help.\n");
                        return -1;
                    }
                    cmd = opt;
                    if (optarg) {
                        set_arg(optarg);
                    }
                } else {
                    printf("error: unsupported option. See --help.\n");
                    return -1;
                }
        }
    }
    if (help) {
        print_help();
        return -1;
    }
    return cmd;
}

int main(int argc, char *argv[]) {
    int cmd; 
    Client client;
    if ((cmd = arg_parse(argc, argv)) < 0) {
        return 0;
    }
    if (!client.init()) {
        return 0;
    }
    client.run_cmd(cmd);       
    client.close();
    return 0;
}
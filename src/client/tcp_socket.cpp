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
#include "tcp_socket.h"
#include <sys/socket.h>
#include <sys/un.h>

bool TcpSocket::recv_msg(Msg &res, MessageHeader &header) {
    MessageProtocol proto;
    if (!handle_request(stream, proto)) {
        printf("can't connect to server!\n");
        return false;
    }
    decode(res, proto.body);
    decode(header, proto.header);
    return true;
}

bool TcpSocket::send_msg(Msg &msg, MessageHeader &header) {
    if (!send_response(stream, msg, header)) {
        return false;
    }
    return true;
}

int TcpSocket::file_connect(const char *name) {
    struct sockaddr_un un;
    int len;
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(name);
    if (connect(sock, (struct sockaddr*)&un, len) < 0) {
        printf("can't connect to server!\n");
        return -1;
    }
    return 0;
}

bool TcpSocket::init() {
    if( (sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        printf("socket create error!\n");
        return false;
    }
    if (file_connect(SOCK_PATH.c_str()) < 0) {
        return false;
    }
    stream.set_sock(sock);
    return true;
}

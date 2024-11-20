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
#include <securec.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace oeaware {
bool TcpSocket::RecvMsg(Message &res, MessageHeader &header)
{
    MessageProtocol proto;
    if (!RecvMessage(stream, proto)) {
        printf("can't connect to server!\n");
        return false;
    }
    res = proto.GetMessage();
    header = proto.GetHeader();
    return true;
}

bool TcpSocket::SendMsg(Message &msg, MessageHeader &header)
{
    MessageProtocol proto;
    proto.SetHeader(header);
    proto.SetMessage(msg);
    if (!SendMessage(stream, proto)) {
        return false;
    }
    return true;
}

int TcpSocket::FileConnect(const char *name)
{
    struct sockaddr_un un;
    int len;
    memset_s(&un, sizeof(un), 0, sizeof(un));
    un.sun_family = AF_UNIX;
    int maxNameLength = 100;
    strcpy_s(un.sun_path, maxNameLength, name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(name);
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&un), len) < 0) {
        printf("can't connect to server!\n");
        return -1;
    }
    return 0;
}

bool TcpSocket::Init(const std::string &path)
{
    sockPath = path;
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        printf("socket create error!\n");
        return -1;
    }
    if (FileConnect(sockPath.c_str()) < 0) {
        return false;
    }
    stream.SetSock(sock);
    return true;
}
}

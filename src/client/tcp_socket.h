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
#ifndef CLIENT_TCP_SOCKET_H
#define CLIENT_TCP_SOCKET_H
#include <unistd.h>
#include "message_protocol.h"
#include "default_path.h"

namespace oeaware {
class TcpSocket {
public:
    TcpSocket() { }
    ~TcpSocket()
    {
        close(sock);
    }
    bool RecvMsg(Message &res, MessageHeader &header);
    bool SendMsg(Message &msg, MessageHeader &header);
    int FileConnect(const char *name);
    bool Init();
private:
    int sock;
    SocketStream stream;
    const std::string sockPath = DEFAULT_RUN_PATH + "/oeAware-sock";
};
}

#endif // !CLIENT_TCP_SOCKET_H
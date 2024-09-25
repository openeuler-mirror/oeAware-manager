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
#include "domain_socket.h"
#include <sys/socket.h>
#include <unistd.h>
#include <securec.h>
#include "default_path.h"

namespace oeaware {
int DomainSocket::CreateSockAddrUn(struct sockaddr_un &un, const std::string &sunPath)
{
    memset_s(&un, sizeof(un), 0, sizeof(un));
    un.sun_family = AF_UNIX;
    int maxNameLength = 100;
    auto ret = strcpy_s(un.sun_path, maxNameLength, sunPath.c_str());
    if (ret != EOK) {
        return -1;
    }
    int len = offsetof(struct sockaddr_un, sun_path)  + sunPath.length();
    return len;
}

int DomainSocket::Socket()
{
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    return sock;
}

int DomainSocket::Bind()
{
    unlink(localPath.c_str());
    int len;
    struct sockaddr_un un;
    if ((len = CreateSockAddrUn(un, localPath)) < 0) {
        return -1;
    }
    if (bind(sock, reinterpret_cast<struct sockaddr*>(&un), len) < 0) {
        return -1;
    }
    return 0;
}

int DomainSocket::Listen()
{
    if (listen(sock, maxRequestNum) < 0) {
        return -1;
    }
    return 0;
}

int DomainSocket::Connect()
{
    struct sockaddr_un un;
    int len;
    if ((len = CreateSockAddrUn(un, remotePath)) < 0) {
        return -1;
    }
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&un), len) < 0) {
        return -1;
    }
    return 0;
}

int DomainSocket::Accept()
{
    return accept(sock, nullptr, nullptr);
}

void DomainSocket::Close()
{
    close(sock);
    sock = 0;
}
}

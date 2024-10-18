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
#ifndef COMMON_DOMAIN_SOCKET_H
#define COMMON_DOMAIN_SOCKET_H
#include <vector>
#include <string>
#include <sys/un.h>
#include <sys/socket.h>

namespace oeaware {
class DomainSocket {
public:
    explicit DomainSocket(const std::string &path) : localPath(path) { }
    DomainSocket() { }
    int Socket();
    int Bind();
    int Listen();
    int Connect();
    int Accept(struct sockaddr_un &addr, socklen_t &len);
    void Close();
    void SetRemotePath(const std::string &path)
    {
        remotePath = path;
    }
    int GetSock()
    {
        return sock;
    }
    std::string GetLocalPath()
    {
        return localPath;
    }
private:
    int CreateSockAddrUn(struct sockaddr_un &un, const std::string &sunPath);
private:
    std::string localPath;
    std::string remotePath;
    int sock;
    const int maxRequestNum = 20;
};
}
#endif

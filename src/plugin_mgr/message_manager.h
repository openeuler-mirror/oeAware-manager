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
#ifndef PLUGIN_MGR_MESSAGE_MANAGER_H
#define PLUGIN_MGR_MESSAGE_MANAGER_H
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "safe_queue.h"
#include "message_protocol.h"
#include "logger.h"
#include "config.h"
#include "event/event.h"

namespace oeaware {
class TcpSocket {
public:
    TcpSocket() : sock(-1), epfd(-1) { }
    ~TcpSocket()
    {
        close(sock);
    }
    bool Init();
    void ServeAccept(std::shared_ptr<SafeQueue<Event>> recvMessage,
        std::shared_ptr<SafeQueue<EventResult>> sendMessage);
private:
    int DomainListen(const char *name);
    void HandleMessage(int curFd, std::shared_ptr<SafeQueue<Event>> recvMessage,
    std::shared_ptr<SafeQueue<EventResult>> sendMessage);
private:
    log4cplus::Logger logger;
    int sock;
    int epfd;
    const int maxRequestNum = 20;
};

class MessageManager {
public:
    MessageManager(const MessageManager&) = delete;
    MessageManager& operator=(const MessageManager&) = delete;
    static MessageManager& GetInstance()
    {
        static MessageManager messageManager;
        return messageManager;
    }
    void Init(std::shared_ptr<SafeQueue<Event>> recvMessage, std::shared_ptr<SafeQueue<EventResult>> sendMessage);
    void Run();
private:
    MessageManager() { }
    void Handler();
    void TcpStart();
private:
    /* Event queue stores Events from the client and is consumed by PluginManager. */
    std::shared_ptr<SafeQueue<Event>> recvMessage;
    /* Event queue stores Events from PluginManager and is consumed by TcpSocket. */
    std::shared_ptr<SafeQueue<EventResult>> sendMessage;
    TcpSocket tcpSocket;
};
}

#endif // !PLUGIN_MGR_MESSAGE_MANAGER_H
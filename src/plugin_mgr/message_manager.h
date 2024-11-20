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
#include <unordered_set>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "message_protocol.h"
#include "logger.h"
#include "config.h"
#include "event.h"
#include "domain_socket.h"

namespace oeaware {
class Epoll {
public:
    void Init();
    bool EventCtl(int op, int eventFd);
    int EventWait(struct epoll_event *events, int maxEvents, int timeout);
    void Close();
private:
    int epfd;
};

class TcpMessageHandler {
public:
    void Init(EventQueue newRecvMessage, EventResultQueue newSendMessage, EventQueue newRecvData);
    void AddConn(int conn, int type);
    bool HandleMessage(int fd);
    void Start();
    void Close();
    bool IsConn(int fd);
    void CloseConn(int fd);
    bool shutdown{false};
private:
    Message GetMessageFromDataEvent(const Event &event);
private:
    /* Use for sdk conn. */
    mutable std::mutex connMutex;
    /* Event queue stores Events from the client and is consumed by PluginManager. */
    EventQueue recvMessage;
    /* Event queue stores Events from PluginManager and is consumed by TcpSocket. */
    EventResultQueue sendMessage;
    /* key:fd, value:type, the first bit of type indicates cmd, the second bit indicates sdk.
       value == 1 indicates cmd connection
       value == 2 indicates sdk connection
       value == -1 indicates disconnected
    */
    std::unordered_map<int, int> conns;
    EventQueue recvData;
    log4cplus::Logger logger;
};

class TcpSocket {
public:
    bool Init(EventQueue recvMessage, EventResultQueue sendMessage, EventQueue newRecvData);
    void ServeAccept();
    void Close();
private:
    void HandleMessage(int fd);
    void InitGroups();
    bool StartListen();
    void SaveConnection();
    void HandleEvents(struct epoll_event *events, int num);
private:
    log4cplus::Logger logger;
    std::unique_ptr<DomainSocket> domainSocket;
    std::unique_ptr<Epoll> epoll;
    TcpMessageHandler tcpMessageHandler;
    std::vector<gid_t> groups;
    const int maxRequestNum = 20;
    const int maxNameLength = 108;
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
    bool Init(EventQueue recvMessage, EventResultQueue sendMessage, EventQueue recvData);
    void Run();
    void Exit();
private:
    MessageManager() { }
    void Handler();
    void TcpStart();
private:
    TcpSocket tcpSocket;
    log4cplus::Logger logger;
};
}

#endif // !PLUGIN_MGR_MESSAGE_MANAGER_H
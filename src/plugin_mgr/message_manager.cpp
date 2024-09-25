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
#include "message_manager.h"
#include <thread>
#include <securec.h>
#include "default_path.h"

namespace oeaware {
int TcpSocket::DomainListen(const char *name)
{
    int len;
    struct sockaddr_un un;
    unlink(name);
    memset_s(&un, sizeof(un), 0, sizeof(un));
    un.sun_family = AF_UNIX;
    int maxNameLength = 100;
    auto ret = strcpy_s(un.sun_path, maxNameLength, name);
    if (ret != EOK) {
        ERROR("[MessageManager] sock path too long!");
        return -1;
    }
    len = offsetof(struct sockaddr_un, sun_path)  + strlen(name);
    if (bind(sock, reinterpret_cast<struct sockaddr*>(&un), len) < 0) {
        ERROR("[MessageManager] bind error!");
        return -1;
    }
    if (chmod(name, S_IRWXU | S_IRGRP | S_IXGRP) == -1) {
        ERROR("[MessageManager] " << name << " chmod error!");
    }
    if (listen(sock, maxRequestNum) < 0) {
        ERROR("[MessageManager] listen error!");
        return -1;
    }
    INFO("[MessageManager] listen : " << name);
    return 0;
}

bool TcpSocket::Init()
{
    CreateDir(DEFAULT_RUN_PATH);
    std::string path = DEFAULT_RUN_PATH + "/oeAware-sock";
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        ERROR("[MessageManager] socket error!");
        return false;
    }
    if (DomainListen(path.c_str()) < 0) {
        return false;
    }
    epfd = epoll_create(1);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);
    if (ret < 0) {
        return false;
    }
    return true;
}

static void SendMsg(Message &msg, std::shared_ptr<SafeQueue<Event>> recvMessage)
{
    Event Event;
    Event.SetOpt(msg.GetOpt());
    Event.SetType(EventType::EXTERNAL);
    for (int i = 0; i < msg.PayloadSize(); ++i) {
        Event.AddPayload(msg.Payload(i));
    }
    recvMessage->Push(Event);
}

static void RecvMsg(Message &msg, std::shared_ptr<SafeQueue<EventResult>> sendMessage)
{
    EventResult res;
    sendMessage->WaitAndPop(res);
    msg.SetOpt(res.GetOpt());
    for (int i = 0; i < res.GetPayloadLen(); ++i) {
        msg.AddPayload(res.GetPayload(i));
    }
}

void TcpSocket::HandleMessage(int curFd, std::shared_ptr<SafeQueue<Event>> recvMessage,
    std::shared_ptr<SafeQueue<EventResult>> sendMessage)
{
    SocketStream stream(curFd);
    MessageProtocol msgProtocol;
    Message clientMsg;
    Message internalMsg;
    if (!RecvMessage(stream, msgProtocol)) {
        epoll_ctl(epfd, EPOLL_CTL_DEL, curFd, NULL);
        close(curFd);
        DEBUG("[MessageManager] one client disconnected!");
        return;
    }
    clientMsg = msgProtocol.GetMessage();
    SendMsg(clientMsg, recvMessage);
    RecvMsg(internalMsg, sendMessage);
    MessageHeader header(MessageType::RESPONSE);
    MessageProtocol resProtocol;
    resProtocol.SetMessage(internalMsg);
    resProtocol.SetHeader(header);
    if (!SendMessage(stream, resProtocol)) {
        WARN("[MessageManager] send msg to client failed!");
    }
}

void TcpSocket::ServeAccept(std::shared_ptr<SafeQueue<Event>> recvMessage,
    std::shared_ptr<SafeQueue<EventResult>> sendMessage)
{
    struct epoll_event evs[MAX_EVENT_SIZE];
    int sz = sizeof(evs) / sizeof(struct epoll_event);
    while (true) {
        int num = epoll_wait(epfd, evs, sz, -1);
        for (int i = 0; i < num; ++i) {
            int curFd = evs[i].data.fd;
            if (curFd == sock) {
                int conn = accept(curFd, NULL, NULL);
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = conn;
                epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev);
                DEBUG("[MessageManager] client connected!");
            } else {
                HandleMessage(curFd, recvMessage, sendMessage);
            }
        }
    }
}

void MessageManager::TcpStart()
{
    if (!tcpSocket.Init()) {
        return;
    }
    tcpSocket.ServeAccept(recvMessage, sendMessage);
}

void MessageManager::Handler()
{
    TcpStart();
}

void MessageManager::Init(std::shared_ptr<SafeQueue<Event>> recvMessage,
    std::shared_ptr<SafeQueue<EventResult>> sendMessage)
{
    this->recvMessage = recvMessage;
    this->sendMessage = sendMessage;
}

void MessageManager::Run()
{
    std::thread t([this]() {
        this->Handler();
    });
    t.detach();
}
}

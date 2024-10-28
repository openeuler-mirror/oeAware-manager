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
#include "utils.h"

namespace oeaware {
static const int CMD_CONN = 1;
static const int SDK_CONN = 2;
void Epoll::Close()
{
    close(epfd);
    epfd = 0;
}
void Epoll::Init()
{
    epfd = epoll_create(1);
}

bool Epoll::EventCtl(int op, int eventFd)
{
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = eventFd;
    return epoll_ctl(epfd, op, eventFd, &ev) == 0;
}

int Epoll::EventWait(struct epoll_event *events, int maxEvents, int timeout)
{
    return epoll_wait(epfd, events, maxEvents, timeout);
}

void TcpSocket::InitGroups()
{
    std::vector<std::string> groupNames{"oeaware", "root"};
    for (auto &groupName : groupNames) {
        auto gid = GetGidByGroupName(groupName);
        if (gid < 0) {
            continue;
        }
         groups.emplace_back(gid);
    }
}

bool TcpSocket::StartListen()
{
    std::string path = domainSocket->GetLocalPath();
    if (domainSocket->Socket() < 0) {
        ERROR(logger, "socket error!");
        return false;
    }
    if (domainSocket->Bind() < 0) {
        ERROR(logger, "bind error!");
        return false;
    }
    if (chmod(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP) == -1) {
        ERROR(logger, path << " chmod error!");
        return false;
    }
    if (domainSocket->Listen() < 0) {
        ERROR(logger, "listen error!");
        return false;
    }
    INFO(logger, "listen : " << path);
    return true;
}

bool TcpSocket::Init(EventQueue newRecvMessage, EventResultQueue newSendMessage, EventQueue newRecvData)
{
    InitGroups();
    logger = Logger::GetInstance().Get("MessageManager");
    tcpMessageHandler.Init(newRecvMessage, newSendMessage, newRecvData);
    CreateDir(DEFAULT_RUN_PATH);
    std::string path = DEFAULT_SERVER_LISTEN_PATH;
    domainSocket = std::make_unique<DomainSocket>(path);
    if (!StartListen()) {
        return false;
    }
    epoll = std::make_unique<Epoll>();
    epoll->Init();
    return epoll->EventCtl(EPOLL_CTL_ADD, domainSocket->GetSock());
}

static bool GetMessageFromRemote(SocketStream &stream, Message &message)
{
    MessageProtocol msgProtocol;
    if (!RecvMessage(stream, msgProtocol)) {
        return false;
    }
    message = msgProtocol.GetMessage();
    return true;
}

static bool SendMessageToRemote(SocketStream &stream, const Message &message)
{
    MessageHeader header(MessageType::RESPONSE);
    MessageProtocol resProtocol;
    resProtocol.SetMessage(message);
    resProtocol.SetHeader(header);
    return SendMessage(stream, resProtocol);
}

static void PushEvent(Message &msg, EventQueue recvMessage, const std::vector<std::string> &extras)
{
    Event event;
    event.opt = msg.opt;
    event.type = EventType::EXTERNAL;
    for (auto &payload : msg.payload) {
        event.payload.emplace_back(payload);
    }
    for (auto &extra : extras) {
        event.payload.emplace_back(extra);
    }
    recvMessage->Push(event);
}

static void GetEventResult(Message &msg, EventResultQueue sendMessage)
{
    EventResult res;
    sendMessage->WaitAndPop(res);
    msg.opt = res.opt;
    for (auto &payload : res.payload) {
        msg.payload.emplace_back(payload);
    }
}

const int DISCONNECTED = -1;
const int DISCONNECTED_AND_UNSUBCRIBE = -2;

void TcpMessageHandler::Init(EventQueue newRecvMessage, EventResultQueue newSendMessage, EventQueue newRecvData)
{
    recvMessage = newRecvMessage;
    sendMessage = newSendMessage;
    recvData = newRecvData;
    logger = Logger::GetInstance().Get("MessageManager");
}

void TcpMessageHandler::AddConn(int conn, int type)
{
    if (conns.count(conn) && conns[conn] > 0) {
        WARN(logger, "conn fd already exists!");
        return;
    }
    conns[conn] = type;
    DEBUG(logger, "sdk connected, fd: " << conn << ".");
}

Message TcpMessageHandler::GetMessageFromDataEvent(const Event &event)
{
    Message msg;
    std::string data = event.payload[1];
    msg.opt = Opt::DATA;
    msg.payload.emplace_back(data);
    return msg;
}

void TcpMessageHandler::Close()
{
    recvData->Push(Event(Opt::SHUTDOWN));
    for (auto &conn : conns) {
        close(conn.first);
    }
}

bool TcpMessageHandler::IsConn(int fd)
{
    return conns[fd] > 0;
}

void TcpMessageHandler::CloseConn(int fd)
{
    conns[fd] = DISCONNECTED;
}

bool TcpMessageHandler::HandleMessage(int fd)
{
    SocketStream stream(fd);
    Message clientMsg;
    Message internalMsg;
    if (!GetMessageFromRemote(stream, clientMsg)) {
        return false;
    }
    if (conns[fd] & SDK_CONN) {
        DEBUG(logger, "sdk message come!");
        PushEvent(clientMsg, recvMessage, {std::to_string(fd)});
    } else {
        PushEvent(clientMsg, recvMessage, {});
    }
    GetEventResult(internalMsg, sendMessage);
    if (conns[fd] & SDK_CONN) {
        std::lock_guard<std::mutex> lock(connMutex);
        return SendMessageToRemote(stream, internalMsg);
    } else {
        return SendMessageToRemote(stream, internalMsg);
    }
}

void TcpMessageHandler::Start()
{
    while (true) {
        Event event;
        recvData->WaitAndPop(event);
        if (event.opt == Opt::SHUTDOWN) {
            break;
        }
        if (event.opt != Opt::DATA) {
            continue;
        }
        int fd = atoi(event.payload[0].c_str());
        if (!conns.count(fd) || conns[fd] == DISCONNECTED) {
            conns[fd] = DISCONNECTED_AND_UNSUBCRIBE;
            recvMessage->Push(Event{Opt::UNSUBSCRIBE, EventType::INTERNAL, {std::to_string(fd)}});
            continue;
        }
        SocketStream stream(fd);
        std::lock_guard<std::mutex> lock(connMutex);
        if (!SendMessageToRemote(stream, GetMessageFromDataEvent(event))) {
             WARN(logger, "data send failed!");
        }
    }
}

void TcpSocket::SaveConnection()
{
    struct sockaddr_un un;
    socklen_t len;
    char name[maxNameLength];
    len = sizeof(un);
    int conn = domainSocket->Accept(un, len);
    if (conn < 0) {
        WARN(logger, "connected failed!");
        return;
    }
    len -= offsetof(struct sockaddr_un, sun_path);
    memcpy_s(name, maxNameLength, un.sun_path, len);
    name[len] = 0;
    bool isSdk = false;
    if (strcmp(name, DEFAULT_SDK_CONN_PATH.c_str()) == 0) {
        isSdk = true;
    }
    // check permission
    if (isSdk && !CheckFileGroups(DEFAULT_SDK_CONN_PATH, groups)) {
        WARN(logger, "sdk permission error");
        return;
    }
    if (!epoll->EventCtl(EPOLL_CTL_ADD, conn)) {
        WARN(logger, "epoll event add failed");
        return;
    }
    int type = 0;
    if (isSdk) {
        type |= SDK_CONN;
    } else {
        type |= CMD_CONN;
    }
    tcpMessageHandler.AddConn(conn, type);
    DEBUG(logger, "client connected!");
}

void TcpSocket::HandleMessage(int fd)
{
    if (!tcpMessageHandler.IsConn(fd)) {
        return;
    }
    if (!tcpMessageHandler.HandleMessage(fd)) {
        tcpMessageHandler.CloseConn(fd);
        epoll->EventCtl(EPOLL_CTL_DEL, fd);
        close(fd);
        DEBUG(logger, "one client disconnected!");
    }
}

void TcpSocket::HandleEvents(struct epoll_event *events, int num)
{
    for (int i = 0; i < num; ++i) {
        int curFd = events[i].data.fd;
        if (curFd == domainSocket->GetSock()) {
            SaveConnection();
        } else {
            HandleMessage(curFd);
        }
    }
}

void TcpSocket::Close()
{
    epoll->Close();
    tcpMessageHandler.Close();
}

void TcpSocket::ServeAccept()
{
    std::thread t([&]() {
        tcpMessageHandler.Start();
    });
    t.detach();
    struct epoll_event evs[MAX_EVENT_SIZE];
    int sz = sizeof(evs) / sizeof(struct epoll_event);
    while (true) {
        auto num = epoll->EventWait(evs, sz, -1);
        if (num <= 0) {
            break;
        }
        HandleEvents(evs, num);
    }
}

void MessageManager::TcpStart()
{
    tcpSocket.ServeAccept();
}

bool MessageManager::Init(EventQueue recvMessage, EventResultQueue sendMessage, EventQueue recvData)
{
    Logger::GetInstance().Register("MessageManager");
    logger = Logger::GetInstance().Get("MessageManager");
    return tcpSocket.Init(recvMessage, sendMessage, recvData);
}

void MessageManager::Exit()
{
    tcpSocket.Close();
}

void MessageManager::Run()
{
    std::thread t([this]() {
        this->TcpStart();
    });
    t.detach();
}
}

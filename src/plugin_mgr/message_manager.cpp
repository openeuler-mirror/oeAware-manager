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
#include <pwd.h>
#include <securec.h>
#include "oeaware/default_path.h"
#include "oeaware/utils.h"

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

static std::vector<std::string> GetUserFromGroup(const std::string &groupName)
{
    std::vector<std::string> users;
    std::ifstream file("/etc/group");
    if (!file.is_open()) {
        return users;
    }
    std::string line;
    size_t userPartIndex = 3;
    while (std::getline(file, line)) {
        std::vector<std::string> parts = SplitString(line, ":");
        if (parts.size() > userPartIndex && parts[0] == groupName) {
            std::vector<std::string> userParts = SplitString(parts[userPartIndex], ",");
            users.insert(users.end(), userParts.begin(), userParts.end());
            break;
        }
    }
    file.close();
    return users;
}

static int GetUid(const std::string &name)
{
    struct passwd pwd;
    struct passwd *result;
    char buf[1024];
    int res = getpwnam_r(name.c_str(), &pwd, buf, sizeof(buf), &result);
    if (res != 0 || result == nullptr) {
        return -1;
    }
    return pwd.pw_uid;
}

void TcpSocket::InitGroups()
{
    std::vector<std::string> groupNames{"oeaware", "root"};
    groups[0].emplace_back(0);
    for (auto &groupName : groupNames) {
        auto gid = GetGidByGroupName(groupName);
        if (gid < 0) {
            continue;
        }
        auto users = GetUserFromGroup(groupName);
        for (auto &user : users) {
            auto uid = GetUid(user);
            if (uid < 0) {
                continue;
            }
            groups[gid].emplace_back(uid);
        }
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
    std::string cmd = "setfacl -m g:oeaware:rw " + path;
    auto ret = system(cmd.c_str());
    if (ret) {
        WARN(logger, "failed to set the communication permission of the oeaware user group.");
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

void TcpMessageHandler::Init(EventQueue newRecvMessage, EventResultQueue newSendMessage, EventQueue newRecvData)
{
    recvMessage = newRecvMessage;
    sendMessage = newSendMessage;
    recvData = newRecvData;
    logger = Logger::GetInstance().Get("MessageManager");
}

void TcpMessageHandler::AddConn(int conn, int type)
{
    std::lock_guard<std::mutex> lock(connMutex);
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
    std::lock_guard<std::mutex> lock(connMutex);
    if (conns[fd] & SDK_CONN) {
        recvMessage->Push(Event{Opt::UNSUBSCRIBE, EventType::INTERNAL, {std::to_string(fd)}});
    }
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
    if (internalMsg.opt == Opt::SHUTDOWN) {
        shutdown = true;
        recvData->Push(Event(Opt::SHUTDOWN));
    }
    DEBUG(logger, "message handle.");
    if (conns[fd] & SDK_CONN) {
        std::lock_guard<std::mutex> lock(connMutex);
        DEBUG(logger, "send response to sdk.");
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
        std::lock_guard<std::mutex> lock(connMutex);
        if (!conns.count(fd) || conns[fd] == DISCONNECTED) {
            continue;
        }
        SocketStream stream(fd);
        if (!SendMessageToRemote(stream, GetMessageFromDataEvent(event))) {
             WARN(logger, "data send failed!");
        }
    }
}

bool TcpSocket::CheckFileGroups(const std::string &path)
{
    struct stat st;
    if (lstat(path.c_str(), &st) < 0) {
        return false;
    }
    for (auto &p : groups) {
        bool ok = std::any_of(p.second.begin(), p.second.end(), [&](uid_t uid) {
            return uid == st.st_uid;
        });
        if (ok) {
            return true;
        }
    }
    return false;
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
    if (len > 0) {
        isSdk = true;
    }
    // check permission
    if (isSdk && !CheckFileGroups(name)) {
        WARN(logger, "sdk permission error, " << name);
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
    if (isSdk) {
        INFO(logger, "a sdk connection is established, " << name);
    }
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
        if (num <= 0 || tcpMessageHandler.shutdown) {
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

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
#include <vector>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "safe_queue.h"
#include "message_protocol.h"
#include "logger.h"
#include "config.h"

namespace oeaware {
enum class MessageType {
    INTERNAL,
    EXTERNAL,
};

class Message {
public:
    Message() : type(MessageType::EXTERNAL) {}
    explicit Message(Opt opt) : opt(opt) {}
    Message(Opt opt, MessageType type) : opt(opt), type(type) {}
    Message(Opt opt, const std::vector<std::string> &payload) : opt(opt), payload(payload) {}
    Opt getOpt()
    {
        return this->opt;
    }
    void SetOpt(Opt newOpt)
    {
        this->opt = newOpt;
    }
    void SetType(MessageType newType)
    {
        this->type = newType;
    }
    MessageType GetType() const
    {
        return this->type;
    }
    void AddPayload(const std::string &s)
    {
        this->payload.emplace_back(s);
    }
    std::string GetPayload(int index) const
    {
        return this->payload[index];
    }
    int GetPayloadLen() const
    {
        return this->payload.size();
    }
private:
    Opt opt;
    MessageType type;
    std::vector<std::string> payload;
};

class TcpSocket {
public:
    TcpSocket() : sock(-1), epfd(-1) { }
    ~TcpSocket()
    {
        close(sock);
    }
    bool Init();
    void ServeAccept(std::shared_ptr<SafeQueue<Message>> handlerMsg, std::shared_ptr<SafeQueue<Message>> resMsg);
private:
    int DomainListen(const char *name);
    void HandleMessage(int curFd, std::shared_ptr<SafeQueue<Message>> handlerMsg,
    std::shared_ptr<SafeQueue<Message>> resMsg);
private:
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
    void Init(std::shared_ptr<SafeQueue<Message>> handlerMsg, std::shared_ptr<SafeQueue<Message>> resMsg);
    void TcpStart();
    void Run();
private:
    MessageManager() { }
private:
    /* Message queue stores messages from the client and is consumed by PluginManager. */
    std::shared_ptr<SafeQueue<Message>> handlerMsg;
    /* Message queue stores messages from PluginManager and is consumed by TcpSocket. */
    std::shared_ptr<SafeQueue<Message>> resMsg;
    TcpSocket tcpSocket;
};
}

#endif // !PLUGIN_MGR_MESSAGE_MANAGER_H
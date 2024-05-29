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

#include "safe_queue.h"
#include "message_protocol.h"
#include "logger.h"
#include "config.h"
#include <vector>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

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
    Opt get_opt() {
        return this->opt;
    }
    void set_opt(Opt opt) {
        this->opt = opt;
    }
    void set_type(MessageType type) {
        this->type = type;
    }
    MessageType get_type() const {
        return this->type;
    }
    void add_payload(std::string s) {
        this->payload.emplace_back(s);
    }
    std::string get_payload(int index) const {
        return this->payload[index];
    }
    int get_payload_len() const {
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
    ~TcpSocket() {
        close(sock);
    }
    bool init();
    void serve_accept(std::shared_ptr<SafeQueue<Message>> handler_msg, std::shared_ptr<SafeQueue<Message>> res_msg);    
private:
    int domain_listen(const char *name);
private:    
    int sock;
    int epfd;
};

class MessageManager {
public:
    MessageManager(const MessageManager&) = delete;
    MessageManager& operator=(const MessageManager&) = delete;
    static MessageManager& get_instance() {
        static MessageManager message_manager;
        return message_manager;
    }
    void init(std::shared_ptr<SafeQueue<Message>> handler_msg, std::shared_ptr<SafeQueue<Message>> res_msg);
    void tcp_start();
    void run();
private:
    MessageManager() { }
private:
    /* Message queue stores messages from the client and is consumed by PluginManager. */
    std::shared_ptr<SafeQueue<Message>> handler_msg;
    /* Message queue stores messages from PluginManager and is consumed by TcpSocket. */
    std::shared_ptr<SafeQueue<Message>> res_msg;
    TcpSocket tcp_socket;
};

#endif // !PLUGIN_MGR_MESSAGE_MANAGER_H
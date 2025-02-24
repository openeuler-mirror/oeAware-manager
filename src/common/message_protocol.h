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
#ifndef COMMON_MESSAGE_PROTOCOL_H
#define COMMON_MESSAGE_PROTOCOL_H
#include <string>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include "oeaware/serialize.h"

namespace oeaware {
const int MAX_RECV_BUFF_SIZE = 16384;
const int MAX_EVENT_SIZE = 1024;
const int PROTOCOL_LENGTH_SIZE = sizeof(size_t);
const int HEADER_LENGTH_SIZE = sizeof(size_t);
const int HEADER_STATE_OK = 0;
const int HEADER_STATE_FAILED = 1;

enum class Opt {
    LOAD,
    ENABLED,
    DISABLED,
    REMOVE,
    QUERY,
    QUERY_ALL,
    QUERY_SUB_GRAPH,
    QUERY_ALL_SUB_GRAPH,
    LIST,
    INFO,
    DOWNLOAD,
    SUBSCRIBE,
    UNSUBSCRIBE,
    PUBLISH,
    DATA,
    RESPONSE_OK,
    RESPONSE_ERROR,
    SHUTDOWN,
};

enum class MessageType {
    REQUEST,
    RESPONSE,
};

struct Message {
    Message() { }
    Message(const Opt &opt, const std::vector<std::string> &payload) : opt(opt), payload(payload) { }
    void Serialize(oeaware::OutStream &out) const
    {
        int op = static_cast<int>(opt);
        out << op << payload;
    }
    void Deserialize(oeaware::InStream &in)
    {
        int op;
        in >> op >> payload;
        opt = static_cast<oeaware::Opt>(op);
    }
    Opt opt;
    std::vector<std::string> payload;
};

class MessageHeader {
public:
    MessageHeader() { }
    explicit MessageHeader(const MessageType &type) : type(type) { }
    MessageType GetMessageType()
    {
        return this->type;
    }
    void Serialize(oeaware::OutStream &out) const
    {
        int iType = static_cast<int>(type);
        out << iType;
    }
    void Deserialize(oeaware::InStream &in)
    {
        int iType;
        in >> iType;
        type = static_cast<MessageType>(iType);
    }
private:
    MessageType type;
};

class MessageProtocol {
public:
    MessageProtocol() { }
    MessageProtocol(const MessageHeader &header, const Message &message) : header(header), message(message) { }
    std::string GetProtocolStr();
    void SetHeader(const std::string &headerStr)
    {
        InStream in(headerStr);
        in >> header;
    }
    void SetMessage(const std::string &content)
    {
        InStream in(content);
        in >> message;
    }
    void SetHeader(const MessageHeader &newHeader)
    {
        this->header = newHeader;
    }
    void SetMessage(const Message &message)
    {
        this->message = message;
    }
    Message GetMessage()
    {
        return message;
    }
    MessageHeader GetHeader()
    {
        return header;
    }
private:
    MessageHeader header;
    Message message;
};

class SocketStream {
public:
    SocketStream() : readBuff(maxBuffSize, 0) { }
    explicit SocketStream(int sock) : sock(sock), readBuff(maxBuffSize, 0) { }
    ~SocketStream()
    {
        readBuff.clear();
        sock = 0;
    }
    ssize_t Read(char buf[], size_t size);
    ssize_t Write(const char buf[], size_t size);
    void SetSock(int newSock)
    {
        this->sock = newSock;
    }
private:
    int sock;
    std::vector<char> readBuff;
    size_t readBuffOff = 0;
    size_t readBuffContentSize = 0;
    static const size_t maxBuffSize = 4096;
};

bool RecvMessage(SocketStream &stream, MessageProtocol &msgProtocol);
bool SendMessage(SocketStream &stream, MessageProtocol &msgProtocol);
}

#endif // !COMMON_MESSAGE_PROTOCOL_H

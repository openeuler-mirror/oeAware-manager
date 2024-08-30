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

#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <sys/socket.h>

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
    QUERY_DEP,
    QUERY_ALL_DEPS,
    LIST,
    DOWNLOAD,
    RESPONSE_OK,
    RESPONSE_ERROR,
    SHUTDOWN,
};

class Msg {
private:
    friend class boost::serialization::access;
    template <typename Archive>
    void serialize(Archive &ar, const unsigned int /* version */)
    {
        ar & opt;
        ar & payload;
    }
public:
    Msg() { }
    int PayloadSize() const
    {
        return this->payload.size();
    }
    std::string Payload(int id) const
    {
        return this->payload[id];
    }
    oeaware::Opt Opt()
    {
        return this->opt;
    }
    void AddPayload(std::string &context)
    {
        this->payload.emplace_back(context);
    }
    void AddPayload(std::string &&context)
    {
        this->payload.emplace_back(context);
    }
    void SetOpt(oeaware::Opt newOpt)
    {
        this->opt = newOpt;
    }
    oeaware::Opt GetOpt() const
    {
        return this->opt;
    }
private:
    oeaware::Opt opt;
    std::vector<std::string> payload;
};

class MessageHeader {
private:
    friend class boost::serialization::access;
    template <typename Archive>
    void serialize(Archive &ar, const unsigned int /* version */)
    {
        ar & code;
    }
public:
    MessageHeader() { }
    int GetStateCode()
    {
        return this->code;
    }
private:
    int code;
};

class MessageProtocol {
public:
    MessageProtocol(): totLength(0), headerLength(0), header(""), body("") { }
    size_t totLength;
    size_t headerLength;
    std::string header;
    std::string body; // Msg serialized data
};

class SocketStream {
public:
    SocketStream() : readBuff(maxBuffSize, 0) { }
    explicit SocketStream(int sock) : sock(sock), readBuff(maxBuffSize, 0) { }
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

bool HandleRequest(SocketStream &stream, MessageProtocol &msgProtocol);
bool SendResponse(SocketStream &stream, const Msg &msg, const MessageHeader &header);

template<typename T>
std::string Encode(const T &msg)
{
    std::stringstream ss;
    boost::archive::binary_oarchive os(ss);
    os << msg;
    return ss.str();
}

template<typename T>
int Decode(T &msg, const std::string &content)
{
    try {
        std::stringstream ss(content);
        boost::archive::binary_iarchive ia(ss);
        ia >> msg;
    } catch (const boost::archive::archive_exception& e) {
        std::cerr << "Serialization failed: " << e.what() << "\n";
        return -1;
    }
    return 0;
}
}

#endif // !COMMON_MESSAGE_PROTOCOL_H

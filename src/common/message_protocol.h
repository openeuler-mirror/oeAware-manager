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
    QUERY_TOP,
    QUERY_ALL_TOP,
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
    void serialize(Archive &ar, const unsigned int version) {
        ar & _opt;
        ar & _payload;
    }
public:
    Msg() { }
    int payload_size() const {
        return this->_payload.size();
    }
    std::string payload(int id) const {
        return this->_payload[id];
    }
    Opt opt() {
        return this->_opt;
    }
    void add_payload(std::string &context) {
        this->_payload.emplace_back(context);
    }
    void add_payload(std::string &&context) {
        this->_payload.emplace_back(context);
    }
    void set_opt(Opt opt) {
        this->_opt = opt;
    }
    Opt get_opt() const {
        return this->_opt;
    }
private:
    Opt _opt;
    std::vector<std::string> _payload;
};

class MessageHeader {
private:
    friend class boost::serialization::access;
    template <typename Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & code;
    }
public:
    MessageHeader() { }
    void set_state_code(int code) {
        this->code = code;
    }
    int get_state_code() {
        return this->code;
    }
private:
    int code;
};

class MessageProtocol {
public:
    MessageProtocol(): tot_length(0), header_length(0), header(""), body("") { }
    size_t tot_length;
    size_t header_length;
    std::string header;
    std::string body; // Msg serialized data
};

class SocketStream {
public:
    SocketStream() : read_buff(MAX_BUFF_SIZE, 0) { }
    SocketStream(int sock) : sock(sock), read_buff(MAX_BUFF_SIZE, 0) { }
    ssize_t read(char *buf, size_t size);
    ssize_t write(const char *buf, size_t size);
    void set_sock(int sock) {
        this->sock = sock;
    }
private:
    int sock;
    std::vector<char> read_buff;
    size_t read_buff_off = 0;
    size_t read_buff_content_size = 0;
    static const size_t MAX_BUFF_SIZE = 4096;
};

bool handle_request(SocketStream &stream, MessageProtocol &msg_protocol);
bool send_response(SocketStream &stream, const Msg &msg, const MessageHeader &header);

template<typename T>
std::string encode(const T &msg) {
    std::stringstream ss;
    boost::archive::binary_oarchive os(ss);
    os << msg;
    return ss.str();
}

template<typename T> 
int decode(T &msg, const std::string &content) {
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

#endif // !COMMON_MESSAGE_PROTOCOL_H
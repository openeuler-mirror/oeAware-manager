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
#include "message_protocol.h"
#include <iostream>

template <typename T>
inline ssize_t handle_EINTR(T fn) {
    ssize_t res = 0;
    while (true) {
        res = fn();
        if (res < 0 && errno == EINTR) {
            continue;
        }
        break;
    }
    return res;
}

inline ssize_t read_socket(int sock, void *ptr, size_t size, int flags) {
    return handle_EINTR([&]() {
        return recv(sock, ptr, size, flags);
    });
}

inline ssize_t send_socket(int sock, const void *ptr, size_t size, int flags) {
    return handle_EINTR([&]() {
        return send(sock, ptr, size, flags);
    });
}

ssize_t SocketStream::read(char *ptr, size_t size) {
    if (read_buff_off < read_buff_content_size) {
        auto remaining_size = read_buff_content_size - read_buff_off;
        if (size <= remaining_size) {
            memcpy(ptr, read_buff.data() + read_buff_off, size);
            read_buff_off += size;
            return size;
        } else {
            memcpy(ptr, read_buff.data() + read_buff_off, remaining_size);
            read_buff_off += remaining_size;
            return remaining_size;
        }
    }
    read_buff_off = 0;
    read_buff_content_size = 0;
    if (size < MAX_BUFF_SIZE) {
        auto n = read_socket(sock, read_buff.data(), MAX_BUFF_SIZE, 0);
        if (n <= 0) {
            return n;
        } else if (n < size) {
            memcpy(ptr, read_buff.data(), n);
            return n;
        } else {
            memcpy(ptr, read_buff.data(), size);
            read_buff_off = size;
            read_buff_content_size = n;
            return size;
        }
    } else {
        return read_socket(sock, ptr, size, 0);
    }
}

ssize_t SocketStream::write(const char *ptr, size_t size) {
    return send_socket(sock, ptr, size, 0);
}

static std::string to_hex(size_t x) {
    std::stringstream ss;
    std::string result;
    ss << std::hex << std::uppercase << x;
    result = ss.str();
    std::string zero = "";
    for (int i = result.length(); i < sizeof(size_t); ++i) {
        zero += "0";
    }
    result = zero + result;
    return result;
}

static size_t from_hex(std::string s) {
    std::stringstream ss;
    size_t ret;
    ss << std::hex << s;
    ss >> ret;
    return ret;
}

std::string encode(Msg &msg) {
    std::stringstream ss;
    boost::archive::binary_oarchive os(ss);
    os << msg;
    return ss.str();
}

int decode(Msg &msg, const std::string &content) {
    int len = 0;
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


bool handle_request(SocketStream &stream, MessageProtocol &msg_protocol) {
    char buf[MAX_RECV_BUFF_SIZE];
    // get tot length
    int ret = stream.read(buf, PROTOCOL_LENGTH_SIZE);
    if (ret <= 0) { 
        return false; 
    }
    int len = from_hex(buf);
    msg_protocol.tot_length = len;
    ret = stream.read(buf, HEADER_LENGTH_SIZE);
    if (ret <= 0) {
        return false;
    }
    msg_protocol.header_length = from_hex(buf);
    int i = PROTOCOL_LENGTH_SIZE + HEADER_LENGTH_SIZE;
    while (i < len) {
        auto read_len = len - i;
        int n = stream.read(buf, std::min(read_len, MAX_RECV_BUFF_SIZE));
        if (n <= 0) { 
            return false;
        }
        msg_protocol.body.append(buf, n);
        i += n;
    }     
    return true;    
}

bool send_response(SocketStream &stream, Msg &msg) {
    MessageProtocol proto;
    proto.body = encode(msg);
    proto.tot_length += PROTOCOL_LENGTH_SIZE;
    proto.tot_length += HEADER_LENGTH_SIZE;
    proto.tot_length += 0; // header data
    proto.tot_length += proto.body.size();
    proto.header_length = 0;
    proto.header = "";
    std::string res = to_hex(proto.tot_length);
    res += to_hex(proto.header_length);
    res += proto.body;
    return stream.write(res.c_str(), res.size());
}
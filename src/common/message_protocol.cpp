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
#include <securec.h>

namespace oeaware {
inline ssize_t ReadSocket(int sock, char buf[], size_t size, int flags)
{
    return recv(sock, buf, size, flags);
}

inline ssize_t SendSocket(int sock, const char buf[], size_t size, int flags)
{
    return send(sock, buf, size, flags);
}

ssize_t SocketStream::Read(char buf[], size_t size)
{
    if (readBuffOff < readBuffContentSize) {
        auto remainingSize = readBuffContentSize - readBuffOff;
        if (size <= remainingSize) {
            auto ret = memcpy_s(buf, MAX_RECV_BUFF_SIZE, readBuff.data() + readBuffOff, size);
            if (ret != EOK) {
                return -1;
            }
            readBuffOff += size;
            return size;
        } else {
            auto ret = memcpy_s(buf, MAX_RECV_BUFF_SIZE, readBuff.data() + readBuffOff, remainingSize);
            if (ret != EOK) {
                return -1;
            }
            readBuffOff += remainingSize;
            return remainingSize;
        }
    }
    readBuffOff = 0;
    readBuffContentSize = 0;
    if (size < maxBuffSize) {
        auto n = ReadSocket(sock, readBuff.data(), maxBuffSize, 0);
        if (n <= 0) {
            return n;
        } else if (static_cast<size_t>(n) < size) {
            auto ret = memcpy_s(buf, MAX_RECV_BUFF_SIZE, readBuff.data(), n);
            if (ret != EOK) {
                return -1;
            }
            return n;
        } else {
            auto ret = memcpy_s(buf, MAX_RECV_BUFF_SIZE, readBuff.data(), size);
            if (ret != EOK) {
                return -1;
            }
            readBuffOff = size;
            readBuffContentSize = n;
            return size;
        }
    } else {
        return ReadSocket(sock, buf, size, 0);
    }
}

ssize_t SocketStream::Write(const char buf[], size_t size)
{
    return SendSocket(sock, buf, size, MSG_NOSIGNAL);
}

std::string MessageProtocol::GetProtocolStr()
{
    OutStream out;
    out << header;
    uint64_t headLength = out.Str().length();
    out << message;
    uint64_t totLength = PROTOCOL_LENGTH_SIZE + HEADER_LENGTH_SIZE + out.Str().length();
    OutStream outLength;
    outLength << totLength << headLength;
    return outLength.Str() + out.Str();
}

static bool ReadBuf(int start, int end, char *buf, SocketStream &stream, std::string &msg)
{
    while (start < end) {
        auto readLen = end - start;
        int n = stream.Read(buf, std::min(readLen, MAX_RECV_BUFF_SIZE));
        if (n <= 0) {
            return false;
        }
        msg.append(buf, n);
        start += n;
    }
    return true;
}

bool RecvMessage(SocketStream &stream, MessageProtocol &msgProtocol)
{
    char buf[MAX_RECV_BUFF_SIZE];
    int ret = stream.Read(buf, PROTOCOL_LENGTH_SIZE + HEADER_LENGTH_SIZE);
    if (ret <= 0) {
        return false;
    }
    InStream in(std::string(buf, ret));
    uint64_t totLength = 0;
    uint64_t headerLength = 0;
    in >> totLength >> headerLength;
    uint64_t headerStart = PROTOCOL_LENGTH_SIZE + HEADER_LENGTH_SIZE;
    uint64_t headerEnd = headerStart + headerLength;
    std::string header;
    if (!ReadBuf(headerStart, headerEnd, buf, stream, header)) {
        return false;
    }
    
    int bodyStart = headerEnd;
    int bodyEnd = totLength;
    std::string message;
    if (!ReadBuf(bodyStart, bodyEnd, buf, stream, message)) {
        return false;
    }
    msgProtocol.SetHeader(header);
    msgProtocol.SetMessage(message);
    return true;
}

bool SendMessage(SocketStream &stream, MessageProtocol &msgProtocol)
{
    auto res = msgProtocol.GetProtocolStr();
    return stream.Write(res.c_str(), res.size());
}
}

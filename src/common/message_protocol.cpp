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

static std::string to_hex(int x) {
    std::stringstream ss;
    std::string result;
    ss << std::hex << std::uppercase << x;
    result = ss.str();
    std::string zero = "";
    for (int i = result.length(); i < LEN; ++i) {
        zero += "0";
    }
    result = zero + result;
    return result;
}

void encode(char *buf, int &len, server::Msg *msg) {
    std::stringstream ss;
    boost::archive::binary_oarchive os(ss);
    std::string body_data;
    std::string s_len;
    int index;
    os << *msg;
    body_data = ss.str();
    len += LEN;
    len += HEAD_LEN;
    len += 0; // header data
    len += body_data.length();

    s_len = to_hex(len);
    index = 0;
    while (index < HEAD_LEN_START_POS) {
        buf[index] = s_len[index];
        index++;
    }

    while (index < HEAD_START_POS) buf[index++] = '0';

    int j = 0;

    while (index < len) {
        buf[index] = body_data[j++];
        index++;
    }
}

int decode(char *buf, server::Msg *msg) {
    int len = 0;
    std::string b;

    for (int i = 0; i < LEN; ++i) {
        int t = 0;
        if (buf[i] >= '0' && buf[i] <= '9') t = buf[i] - '0';
        else t = buf[i] - 'A' + 10;
        len = len * 16 + t;
    }
    
    for (int i = HEAD_START_POS; i < len; ++i) {
        b += buf[i];
    }
    try {
        std::stringstream ss(b);
        boost::archive::binary_iarchive ia(ss);
        ia >> (*msg);
    } catch (const boost::archive::archive_exception& e) {
        std::cerr << "Serialization failed: " << e.what() << "\n";
    }
    return 0;
}
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
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <iostream>
#include <sstream>
#include "message.h"
/*
 * Message protocol is as follows:
 *       4               4           
 * | total length | header length | header data | body data |
 *
 */
const int MAX_BUFF_SIZE = 4096;
const int MAX_EVENT_SIZE = 1024;
const int LEN = 4;
const int HEAD_LEN = 4;
const int HEAD_LEN_START_POS = 4;
const int HEAD_START_POS = 8;

void encode(char *buf, int &len, server::Msg *msg);

int decode(char *buf, server::Msg *msg);

#endif // !COMMON_MESSAGE_PROTOCOL_H
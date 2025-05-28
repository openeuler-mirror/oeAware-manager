/******************************************************************************
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef OE_NET_FLOW_COMM_H
#define OE_NET_FLOW_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

struct SockKey {
    uint32_t localIp;
    uint32_t remoteIp;
    uint16_t localPort;
    uint16_t remotePort;
};

struct ThreadData {
    uint32_t pid;
    uint32_t tid;
    char comm[16];
};

struct SockInfo {
    struct ThreadData client;
    struct ThreadData server;
    uint64_t flow;
};

struct QueueInfo {
    struct ThreadData td;
    int queueId;
    int ifIdx;
    uint64_t times;
    uint64_t len;
};

#ifdef __cplusplus
}
#endif

#endif
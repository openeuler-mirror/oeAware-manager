/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef __PLUGIN_COMM_H__
#define __PLUGIN_COMM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CYCLES_COUNTING_BUF_SIZE         10
#define CYCLES_SAMPLING_BUF_SIZE         10
#define UNCORE_BUF_SIZE                  10
#define SPE_BUF_SIZE                     10
#define NETIF_RX_BUF_SIZE                10
#define NAPI_GRO_REC_ENTRY_BUF_SIZE      10
#define SKB_COPY_DATAGRAM_IOVEC_BUF_SIZE 10
#define NET_RECEIVE_TRACE_SAMPLE_PERIOD  10

struct DataRingBuf;
struct PmuData;

struct DataRingBuf *init_buf(int buf_len, const char *instance_name);
void free_buf(struct DataRingBuf *data_ringbuf);
void fill_buf(struct DataRingBuf *data_ringbuf, struct PmuData *pmu_data, int len);

#ifdef __cplusplus
}
#endif

#endif

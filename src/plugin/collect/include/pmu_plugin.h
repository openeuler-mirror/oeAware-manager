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
#ifndef __PMU_PLUGIN_H__
#define __PMU_PLUGIN_H__

#include <securec.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#ifdef __cplusplus
extern "C" {
#endif

#define PMU_CYCLES_COUNTING "pmu_cycles_counting"
#define PMU_CYCLES_SAMPLING "pmu_cycles_sampling"
#define PMU_UNCORE "pmu_uncore_counting"
#define PMU_SPE "pmu_spe_sampling"
#define PMU_NETIF_RX "pmu_netif_rx_counting"
#define PMU_NAPI_GRO_REC_ENTRY "pmu_napi_gro_rec_entry"
#define PMU_SKB_COPY_DATEGRAM_IOVEC "pmu_skb_copy_datagram_iovec"
    
#define NAPI_GRO_REC_ENTRY_DEVICE_LEN 64
// ref : /sys/kernel/debug/tracing/events/net/napi_gro_receive_entry/format
struct NapiGroRecEntryData {
    unsigned short commonType;
    unsigned char commonFlags;
    unsigned char commonPreemptCount;
    int commonPid;

    int dataLocName;
    unsigned int napiId;
    uint64_t queueMapping;
    const void *skbaddr;
    bool vlanTagged;
    uint64_t vlanProto;
    uint64_t vlanTci;
    uint64_t protocol;
    uint8_t ipSummed;
    uint32_t hash;
    bool l4Hash;
    unsigned int len;
    unsigned int dataLen;
    unsigned int truesize;
    bool macHeaderValid;
    int macHeader;
    unsigned char nrFrags;
    uint64_t gsoSize;
    uint64_t gsoType;

    char deviceName[NAPI_GRO_REC_ENTRY_DEVICE_LEN];
};
static inline errno_t NapiGroRecEntryResolve(char *raw, struct NapiGroRecEntryData *data)
{
    int nameLen = NAPI_GRO_REC_ENTRY_DEVICE_LEN - 1;
    int dataLen = sizeof(struct NapiGroRecEntryData) - sizeof(data->deviceName);
    errno_t err = memcpy_s(data, dataLen, raw, dataLen);
    if (err) {
        return err;
    }
    char *deviceName = raw + (unsigned short)(data->dataLocName & 0xffff);
    if (strlen(deviceName) + 1 < NAPI_GRO_REC_ENTRY_DEVICE_LEN) {
        nameLen = strlen(deviceName) + 1;
    }
    return memcpy_s(data->deviceName, nameLen, deviceName, nameLen);
}
// ref : /sys/kernel/debug/tracing/events/skb/skb_copy_datagram_iovec/format
struct SkbCopyDatagramIovecData {
    unsigned short commonType;
    unsigned char commonFlags;
    unsigned char commonPreemptCount;
    int commonPid;
    const void *skbaddr;
    int len;
};

static inline errno_t SkbCopyDatagramIovecResolve(char *raw, struct SkbCopyDatagramIovecData *data)
{
    return memcpy_s(data, sizeof(struct SkbCopyDatagramIovecData), raw, sizeof(struct SkbCopyDatagramIovecData));
}

#ifdef __cplusplus
}
#endif

#endif

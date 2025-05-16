/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/

#ifndef OEAWARE_NETWORK_INTERFACE_H
#define OEAWARE_NETWORK_INTERFACE_H

#include <net/if.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OE_NETWORK_INTERFACE_BASE_TOPIC "base"
struct NetworkInterfaceData {
    char name[IFNAMSIZ];                             // interface name (IFNAMSIZ = 16, include\uapi\linux\if.h)
    /*
    * https://www.kernel.org/doc/html/latest/networking/operstates.html
    * IF_OPER_UNKNOWN,
	* IF_OPER_NOTPRESENT,
	* IF_OPER_DOWN,
	* IF_OPER_LOWERLAYERDOWN,
	* IF_OPER_TESTING,
	* IF_OPER_DORMANT,
	* IF_OPER_UP,
    */
    int operstate;                                    // interface state
    int ifindex;                                      // interface index
};

typedef struct {
    int count;
    struct NetworkInterfaceData *base;
} NetIntfBaseDataList;

#define OE_NETWORK_INTERFACE_DRIVER_TOPIC "driver" // ref: ethtool -i eth
#define OE_NET_DRIVER_DATA_LEN 32
#define OE_NET_DRIVER_VER_DATA_LEN 32
#define OE_NET_DRIVER_FW_VER_DATA_LEN 32
#define OE_NET_DRIVER_BUS_INFO_DATA_LEN 32
// ref: ethtool -i eth(struct ethtool_drvinfo, include\uapi\linux\ethtool.h)
struct NetworkInterfaceDriverData {
    char name[IFNAMSIZ];
    char driver[OE_NET_DRIVER_DATA_LEN];
    char version[OE_NET_DRIVER_VER_DATA_LEN];
    char fwVersion[OE_NET_DRIVER_FW_VER_DATA_LEN];
    char busInfo[OE_NET_DRIVER_BUS_INFO_DATA_LEN];
};

// not include eth which ethtool -i eth not support
typedef struct {
    int count;
    struct NetworkInterfaceDriverData *driver;
} NetIntfDriverDataList;

// topic
#define OE_LOCAL_NET_AFFINITY "local_net_affinity"
// params of local_net_affinity
#define OE_PARA_LOC_NET_AFFI_USER_DEBUG "local_net_affinity_usr_debug"
#define OE_PARA_LOC_NET_AFFI_KERN_DEBUG "local_net_affinity_kern_debug"
#define OE_PARA_PROCESS_AFFINITY "process_affinity"
struct ProcessNetAffinityData {
    uint32_t pid1;
    uint32_t pid2;
    uint64_t level;
};

typedef struct {
    int count;
    struct ProcessNetAffinityData *affinity;
} ProcessNetAffinityDataList;

#ifdef __cplusplus
}
#endif

#endif
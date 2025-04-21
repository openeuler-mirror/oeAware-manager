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
#ifndef NET_INTERFACE_COMM_H_
#define NET_INTERFACE_COMM_H_

#include <unordered_set>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <securec.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <sys/ioctl.h>
#include <linux/if.h>

struct NetIntfBaseInfo {
    std::string name = "";
    int operstate = IF_OPER_UNKNOWN;
};

std::unordered_set<std::string> GetNetInterface();
bool ReadEthtoolDrvinfo(const std::string &name, struct ethtool_drvinfo &drvinfo);
void GetNetIntfBaseInfo(const std::string &name, NetIntfBaseInfo &info);
#endif
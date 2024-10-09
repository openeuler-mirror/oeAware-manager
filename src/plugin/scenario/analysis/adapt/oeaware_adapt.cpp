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
#include <string>
#include <iostream>
 /* oeaware manager interface */
#include "interface.h"
/* dependent external plugin interfaces */
#include "pmu_plugin.h"
/* external plugin dependent interfaces */

/* internal data processing interface */
#include "analysis.h"

static std::unordered_map<std::string, uint64_t> g_lastBufCnt;
static std::unordered_map<std::string, const DataRingBuf *> g_ringBufMap;

static std::string g_deps;
static Analysis g_analysis;
static const char *GetVersion()
{
    return nullptr;
}

static const char *GetName()
{
    return "analysis";
}

static const char *GetDescription()
{
    return nullptr;
}

static const char *GetDep()
{
    g_deps = PMU_SPE;
    g_deps += "-";
    g_deps += PMU_CYCLES_SAMPLING;
    g_deps += "-";
    g_deps += PMU_NETIF_RX;
    return g_deps.c_str();
}

static int GetPriority()
{
    return 1;
}

static int GetPeriod()
{
    return g_analysis.GetPeriod();
}

static bool Enable()
{
    g_analysis.Init();
    return true;
}
static void Disable()
{
    g_analysis.Exit();
}

static const struct DataRingBuf *GetRingBuf()
{
    return nullptr;
}

static int GetRingBufNum(const std::string &instanceName)
{
    const DataRingBuf *ringBufs = g_ringBufMap[instanceName];
    if (ringBufs == nullptr) {
        return 0;
    }
    return std::min(ringBufs->count - g_lastBufCnt[instanceName], (uint64_t)ringBufs->buf_len);
}

static void UpdateBufNPmu(int n, const std::string &instanceName)
{
    const DataRingBuf *ringBufs = g_ringBufMap[instanceName];
    if (ringBufs == nullptr) {
        return;
    }
    int offset = (ringBufs->index + ringBufs->buf_len - n) % ringBufs->buf_len;
    DataBuf *buf = &ringBufs->buf[offset];
    if (buf && buf->len > 0) {
        g_analysis.UpdatePmu(instanceName, buf->len, (PmuData *)buf->data);
    }
}

static void UpdateBufCnt(const std::string &instanceName)
{
    if (g_ringBufMap[instanceName]) {
        g_lastBufCnt[instanceName] = g_ringBufMap[instanceName]->count;
    } else {
        g_lastBufCnt[instanceName] = 0; // reset
    }
}

static void UpdatePmu()
{
    int speBufNum = GetRingBufNum(PMU_SPE);
    // spe and cycles should have same ring buf num
    if (speBufNum != 0 && speBufNum == GetRingBufNum(PMU_CYCLES_SAMPLING) && speBufNum == GetRingBufNum(PMU_NETIF_RX)) {
        for (int n = 0; n < speBufNum; ++n) {
            UpdateBufNPmu(n, PMU_CYCLES_SAMPLING);
            UpdateBufNPmu(n, PMU_SPE);
            UpdateBufNPmu(n, PMU_NETIF_RX);
            // to do something after update spe and cycles every time
        }
    }
    UpdateBufCnt(PMU_SPE);
    UpdateBufCnt(PMU_CYCLES_SAMPLING);
    UpdateBufCnt(PMU_NETIF_RX);
}

static void Run(const Param *param)
{
    for (int i = 0; i < param->len; ++i) {
        if (param->ring_bufs[i]) {
            g_ringBufMap[param->ring_bufs[i]->instance_name] = param->ring_bufs[i];
        }
    }
    UpdatePmu();
    g_analysis.Analyze();
    g_ringBufMap.clear();
}

struct Interface g_analysisIntf = {
    .get_version = GetVersion,
    .get_name = GetName,
    .get_description = GetDescription,
    .get_dep = GetDep,
    .get_priority = GetPriority,
    .get_type = nullptr,
    .get_period = GetPeriod,
    .enable = Enable,
    .disable = Disable,
    .get_ring_buf = GetRingBuf,
    .run = Run,
};

extern "C" int get_instance(Interface **ins)
{
    *ins = &g_analysisIntf;
    return 1;
}
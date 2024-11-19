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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <securec.h>
#include "pmu.h"
#include "pcerrc.h"
#include "interface.h"
#include "pmu_plugin.h"
#include "plugin_comm.h"
#include "plugin_sampling.h"

static bool g_samplingIsOpen = false;
static int g_samplingPd = -1;
static struct DataRingBuf *g_samplingBuf = NULL;
struct PmuData *g_pmuData = NULL;

static int Init()
{
    g_samplingBuf = init_buf(NAPI_GRO_REC_ENTRY_BUF_SIZE, PMU_NAPI_GRO_REC_ENTRY);
    if (!g_samplingBuf) {
        return -1;
    }

    return 0;
}

static void Finish()
{
    if (!g_samplingBuf) {
        return;
    }

    free_buf(g_samplingBuf);
    g_samplingBuf = NULL;
}

static int Open()
{
    struct PmuAttr attr;
    char *evtList[1];
    int pd;

    (void)memset_s(&attr, sizeof(struct PmuAttr), 0, sizeof(struct PmuAttr));

    evtList[0] = "net:napi_gro_receive_entry";

    attr.evtList = evtList;
    attr.numEvt = 1;
    attr.pidList = NULL;
    attr.numPid = 0;
    attr.cpuList = NULL;
    attr.numCpu = 0;
    attr.period = NET_RECEIVE_TRACE_SAMPLE_PERIOD;

    pd = PmuOpen(SAMPLING, &attr);
    if (pd == -1) {
        printf("%s\n", Perror());
        return pd;
    }

    g_samplingIsOpen = true;
    return pd;
}

static void Close()
{
    PmuClose(g_samplingPd);
    g_samplingPd = -1;
    g_samplingIsOpen = false;
}

bool NapiGroRecEntryEnable()
{
    if (!g_samplingBuf) {
        int ret = Init();
        if (ret != 0) {
            goto err;
        }
    }

    if (!g_samplingIsOpen) {
        g_samplingPd = Open();
        if (g_samplingPd == -1) {
            Finish();
            goto err;
        }
    }

    return PmuEnable(g_samplingPd) == 0;

err:
    return false;
}

void NapiGroRecEntryDisable()
{
    PmuDisable(g_samplingPd);
    Close();
    Finish();
}

const struct DataRingBuf *NapiGroRecEntryGetBuf()
{
    return (const struct DataRingBuf *)g_samplingBuf;
}

static void NapiGroRecEntryReflashBuf()
{
    struct DataRingBuf *dataRingBuf;
    int len;

    dataRingBuf = (struct DataRingBuf *)g_samplingBuf;
    if (!dataRingBuf) {
        printf("g_samplingBuf has not malloc\n");
        return;
    }

    PmuDisable(g_samplingPd);
    len = PmuRead(g_samplingPd, &g_pmuData);
    PmuEnable(g_samplingPd);
    fill_buf(dataRingBuf, g_pmuData, len);
}

void NapiGroRecEntryRun(const struct Param *param)
{
    (void)param;
    NapiGroRecEntryReflashBuf();
}

const char *NapiGroRecEntryGetVer()
{
    return NULL;
}

const char *NapiGroRecEntryGetName()
{
    return PMU_NAPI_GRO_REC_ENTRY;
}

const char *NapiGroRecEntryGetDes()
{
    return "event used to collect net queue info";
}

const char *NapiGroRecEntryGetDep()
{
    return NULL;
}

int NapiGroRecEntryGetPriority()
{
    return 0;
}

int NapiGroRecEntryGetType()
{
    return -1;
}

int NapiGroRecEntryGetPeriod()
{
    return 100; // 100ms
}

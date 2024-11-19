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

static bool sampling_is_open = false;
static int sampling_pd = -1;
static struct DataRingBuf *sampling_buf = NULL;
struct PmuData *sampling_data = NULL;

static int sampling_init()
{
    sampling_buf = init_buf(CYCLES_SAMPLING_BUF_SIZE, PMU_CYCLES_SAMPLING);
    if (!sampling_buf) {
        return -1;
    }

    return 0;
}

static void sampling_fini()
{
    if (!sampling_buf) {
        return;
    }

    free_buf(sampling_buf);
    sampling_buf = NULL;
}

static int sampling_open()
{
    struct PmuAttr attr;
    char *evtList[1];
    int pd;

    (void)memset_s(&attr, sizeof(struct PmuAttr), 0, sizeof(struct PmuAttr));

    evtList[0] = "cycles";

    attr.evtList = evtList;
    attr.numEvt = 1;
    attr.pidList = NULL;
    attr.numPid = 0;
    attr.cpuList = NULL;
    attr.numCpu = 0;
    attr.freq = 100;
    attr.useFreq = 1;

    pd = PmuOpen(SAMPLING, &attr);
    if (pd == -1) {
        printf("%s\n", Perror());
        return pd;
    }

    sampling_is_open = true;
    return pd;
}

static void sampling_close()
{
    PmuClose(sampling_pd);
    sampling_pd = -1;
    sampling_is_open = false;
}

bool sampling_enable()
{
    if (!sampling_buf) {
        int ret = sampling_init();
        if (ret != 0) {
            goto err;
        }
    }

    if (!sampling_is_open) {
        sampling_pd = sampling_open();
        if (sampling_pd == -1) {
            sampling_fini();
            goto err;
        }
    }

    return PmuEnable(sampling_pd) == 0;

err:
    return false;
}

void sampling_disable()
{
    PmuDisable(sampling_pd);
    sampling_close();
    sampling_fini();
}

const struct DataRingBuf *sampling_get_ring_buf()
{
    return (const struct DataRingBuf *)sampling_buf;
}

static void sampling_reflash_ring_buf()
{
    struct DataRingBuf *data_ringbuf;
    int len;

    data_ringbuf = (struct DataRingBuf *)sampling_buf;
    if (!data_ringbuf) {
        printf("sampling_buf has not malloc\n");
        return;
    }

    PmuDisable(sampling_pd);
    len = PmuRead(sampling_pd, &sampling_data);
    PmuEnable(sampling_pd);

    fill_buf(data_ringbuf, sampling_data, len);
}

void sampling_run(const struct Param *param)
{
    (void)param;
    sampling_reflash_ring_buf();
}

const char *sampling_get_version()
{
    return NULL;
}

const char *sampling_get_name()
{
    return PMU_CYCLES_SAMPLING;
}

const char *sampling_get_description()
{
    return NULL;
}

const char *sampling_get_dep()
{
    return NULL;
}

int sampling_get_priority()
{
    return 0;
}

int sampling_get_type()
{
    return -1;
}

int sampling_get_period()
{
    return 100;
}

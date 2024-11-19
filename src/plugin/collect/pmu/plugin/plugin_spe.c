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
#include "plugin_spe.h"

static bool spe_is_open = false;
static int spe_pd = -1;
static struct DataRingBuf *spe_buf = NULL;
struct PmuData *spe_data = NULL;

static int spe_init()
{
    spe_buf = init_buf(SPE_BUF_SIZE, PMU_SPE);
    if (!spe_buf) {
        return -1;
    }

    return 0;
}

static void spe_fini()
{
    if (!spe_buf) {
        return;
    }

    free_buf(spe_buf);
    spe_buf = NULL;
}

static int spe_open()
{
    struct PmuAttr attr;
    int pd;

    (void)memset_s(&attr, sizeof(struct PmuAttr), 0, sizeof(struct PmuAttr));

    attr.evtList = NULL;
    attr.numEvt = 0;
    attr.pidList = NULL;
    attr.numPid = 0;
    attr.cpuList = NULL;
    attr.numCpu = 0;
    attr.period = 2048;
    attr.dataFilter = SPE_DATA_ALL;
    attr.evFilter = SPE_EVENT_RETIRED;
    attr.minLatency = 0x60;

    pd = PmuOpen(SPE_SAMPLING, &attr);
    if (pd == -1) {
        printf("%s\n", Perror());
        return pd;
    }

    spe_is_open = true;
    return pd;
}

static void spe_close()
{
    PmuClose(spe_pd);
    spe_pd = -1;
    spe_is_open = false;
}

bool spe_enable()
{
    if (!spe_buf) {
        int ret = spe_init();
        if (ret != 0) {
            goto err;
        }
    }

    if (!spe_is_open) {
        spe_pd = spe_open();
        if (spe_pd == -1) {
            spe_fini();
            goto err;
        }
    }

    return PmuEnable(spe_pd) == 0;

err:
    return false;
}

void spe_disable()
{
    PmuDisable(spe_pd);
    spe_close();
    spe_fini();
}

const struct DataRingBuf *spe_get_ring_buf()
{
    return (const struct DataRingBuf *)spe_buf;
}

static void spe_reflash_ring_buf()
{
    struct DataRingBuf *data_ringbuf;
    int len;

    data_ringbuf = (struct DataRingBuf *)spe_buf;
    if (!data_ringbuf) {
        printf("spe_buf has not malloc\n");
        return;
    }

    // while using PMU_SPE, PmuRead internally calls PmuEnable and PmuDisable
    len = PmuRead(spe_pd, &spe_data);

    fill_buf(data_ringbuf, spe_data, len);
}

void spe_run(const struct Param *param)
{
    (void)param;
    spe_reflash_ring_buf();
}

const char *spe_get_version()
{
    return NULL;
}

const char *spe_get_name()
{
    return PMU_SPE;
}

const char *spe_get_description()
{
    return NULL;
}

const char *spe_get_dep()
{
    return NULL;
}

int spe_get_priority()
{
    return 0;
}

int spe_get_type()
{
    return -1;
}

int spe_get_period()
{
    return 100;
}

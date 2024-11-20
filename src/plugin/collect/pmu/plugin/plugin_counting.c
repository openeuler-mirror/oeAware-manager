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
#include "plugin_counting.h"

static bool counting_is_open = false;
static int counting_pd = -1;
static struct DataRingBuf *counting_buf = NULL;
struct PmuData *counting_data = NULL;

static int counting_init()
{
    counting_buf = init_buf(CYCLES_COUNTING_BUF_SIZE, PMU_CYCLES_COUNTING);
    if (!counting_buf) {
        return -1;
    }

    return 0;
}

static void counting_fini()
{
    if (!counting_buf) {
        return;
    }

    free_buf(counting_buf);
    counting_buf = NULL;
}

static int counting_open()
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

    pd = PmuOpen(COUNTING, &attr);
    if (pd == -1) {
        printf("%s\n", Perror());
        return pd;
    }

    counting_is_open = true;
    return pd;
}

static void counting_close()
{
    PmuClose(counting_pd);
    counting_pd = -1;
    counting_is_open = false;
}

bool counting_enable()
{
    if (!counting_buf) {
        int ret = counting_init();
        if (ret != 0) {
            goto err;
        }
    }

    if (!counting_is_open) {
        counting_pd = counting_open();
        if (counting_pd == -1) {
            counting_fini();
            goto err;
        }
    }

    return PmuEnable(counting_pd) == 0;

err:
    return false;
}

void counting_disable()
{
    PmuDisable(counting_pd);
    counting_close();
    counting_fini();
}

const struct DataRingBuf *counting_get_ring_buf()
{
    return (const struct DataRingBuf *)counting_buf;
}

static void counting_reflash_ring_buf()
{
    struct DataRingBuf *data_ringbuf;
    int len;

    data_ringbuf = (struct DataRingBuf *)counting_buf;
    if (!data_ringbuf) {
        printf("counting_buf has no malloc\n");
        return;
    }

    PmuDisable(counting_pd);
    len = PmuRead(counting_pd, &counting_data);
    PmuEnable(counting_pd);

    fill_buf(data_ringbuf, counting_data, len);
}

void counting_run(const struct Param *param)
{
    (void)param;
    counting_reflash_ring_buf();
}

const char *counting_get_version()
{
    return NULL;
}

const char *counting_get_name()
{
    return PMU_CYCLES_COUNTING;
}

const char *counting_get_description()
{
    return NULL;
}

const char *counting_get_dep()
{
    return NULL;
}

int counting_get_priority()
{
    return 0;
}

int counting_get_type()
{
    return -1;
}

int counting_get_period()
{
    return 100;
}

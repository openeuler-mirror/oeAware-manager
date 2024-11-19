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
#include "plugin_uncore.h"
#include "pmu_uncore.h"

static bool uncore_is_open = false;
static int uncore_pd = -1;
static struct DataRingBuf *uncore_buf = NULL;
struct PmuData *uncore_data = NULL;

static int uncore_init()
{
    uncore_buf = init_buf(UNCORE_BUF_SIZE, PMU_UNCORE);
    if (!uncore_buf) {
        return -1;
    }

    return 0;
}

static void uncore_fini()
{
    if (!uncore_buf) {
        return;
    }

    free_buf(uncore_buf);
    uncore_buf = NULL;
}

static int uncore_open()
{
    struct PmuAttr attr;
    struct uncore_config *rx_outer;
    struct uncore_config *rx_sccl;
    struct uncore_config *rx_ops_num;
    int hha_num;
    int pd = -1;
    int ret;

    // Base on oeAware framework, uncore_open is called within uncore_enable.
    // If pmu_uncore is not supported, it will generate a large number of error logs.
    // So temporarily set uncore_is_open = true util oeAware framework provides open API.
    uncore_is_open = true;

    ret = hha_uncore_config_init();
    if (ret != 0) {
        printf("This system not support pmu_uncore\n");
        return pd;
    }

    hha_num = get_uncore_hha_num();
    rx_outer = get_rx_outer();
    rx_sccl = get_rx_sccl();
    rx_ops_num = get_rx_ops_num();

    char *evtList[hha_num * UNCORE_MAX];
    for (int i = 0; i < hha_num; i++) {
        evtList[i + hha_num * RX_OUTER] = rx_outer[i].uncore_name;
        evtList[i + hha_num * RX_SCCL] = rx_sccl[i].uncore_name;
        evtList[i + hha_num * RX_OPS_NUM] = rx_ops_num[i].uncore_name;
    }

    (void)memset_s(&attr, sizeof(struct PmuAttr), 0, sizeof(struct PmuAttr));

    attr.evtList = evtList;
    attr.numEvt = hha_num * UNCORE_MAX;
    attr.pidList = NULL;
    attr.numPid = 0;
    attr.cpuList = NULL;
    attr.numCpu = 0;

    pd = PmuOpen(COUNTING, &attr);
    if (pd == -1) {
        printf("%s\n", Perror());
        return pd;
    }

    return pd;
}

static void uncore_close()
{
    PmuClose(uncore_pd);
    uncore_pd = -1;
    uncore_is_open = false;
}

bool uncore_enable()
{
    if (!uncore_buf) {
        int ret = uncore_init();
        if (ret != 0) {
            goto err;
        }
    }

    if (!uncore_is_open) {
        uncore_pd = uncore_open();
        if (uncore_pd == -1) {
            uncore_fini();
            goto err;
        }
    }

    return PmuEnable(uncore_pd) == 0;

err:
    return false;
}

void uncore_disable()
{
    PmuDisable(uncore_pd);
    uncore_close();
    uncore_fini();
}

const struct DataRingBuf *uncore_get_ring_buf()
{
    return (const struct DataRingBuf *)uncore_buf;
}

static void uncore_reflash_ring_buf()
{
    struct DataRingBuf *data_ringbuf;
    int len;

    data_ringbuf = (struct DataRingBuf *)uncore_buf;
    if (!data_ringbuf) {
        printf("uncore_buf has not malloc\n");
        return;
    }

    PmuDisable(uncore_pd);
    len = PmuRead(uncore_pd, &uncore_data);
    PmuEnable(uncore_pd);

    fill_buf(data_ringbuf, uncore_data, len);
}

void uncore_run(const struct Param *param)
{
    (void)param;
    uncore_reflash_ring_buf();
}

const char *uncore_get_version()
{
    return NULL;
}

const char *uncore_get_name()
{
    return PMU_UNCORE;
}

const char *uncore_get_description()
{
    return NULL;
}

const char *uncore_get_dep()
{
    return NULL;
}

int uncore_get_priority()
{
    return 0;
}

int uncore_get_type()
{
    return -1;
}

int uncore_get_period()
{
    return 100;
}

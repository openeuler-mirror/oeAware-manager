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

static bool event_is_open = false;
static int pmu_id = -1;
static struct DataRingBuf *ring_buf = NULL;
struct PmuData *pmu_data = NULL;

static int init()
{
    ring_buf = init_buf(NETIF_RX_BUF_SIZE, PMU_NETIF_RX);
    if (!ring_buf) {
        return -1;
    }

    return 0;
}

static void finish()
{
    if (!ring_buf) {
        return;
    }

    free_buf(ring_buf);
    ring_buf = NULL;
}

static int open()
{
    struct PmuAttr attr;
    char *evtList[1];
    int pd;

    (void)memset_s(&attr, sizeof(struct PmuAttr), 0, sizeof(struct PmuAttr));

    evtList[0] = "net:netif_rx";

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

    event_is_open = true;
    return pd;
}

static void netif_rx_close()
{
    PmuClose(pmu_id);
    pmu_id = -1;
    event_is_open = false;
}

bool netif_rx_enable()
{
    if (!ring_buf) {
        int ret = init();
        if (ret != 0) {
            goto err;
        }
    }

    if (!event_is_open) {
        pmu_id = open();
        if (pmu_id == -1) {
            finish();
            goto err;
        }
    }

    return PmuEnable(pmu_id) == 0;

err:
    return false;
}

void netif_rx_disable()
{
    PmuDisable(pmu_id);
    netif_rx_close();
    finish();
}

const struct DataRingBuf *netif_rx_get_ring_buf()
{
    return (const struct DataRingBuf *)ring_buf;
}

static void reflash_ring_buf()
{
    struct DataRingBuf *data_ringbuf;
    int len;

    data_ringbuf = (struct DataRingBuf *)ring_buf;
    if (!data_ringbuf) {
        printf("ring_buf has not malloc\n");
        return;
    }

    PmuDisable(pmu_id);
    len = PmuRead(pmu_id, &pmu_data);
    PmuEnable(pmu_id);

    fill_buf(data_ringbuf, pmu_data, len);
}

void netif_rx_run(const struct Param *param)
{
    (void)param;
    reflash_ring_buf();
}

const char *netif_rx_get_version()
{
    return NULL;
}

const char *netif_rx_get_name()
{
    return PMU_NETIF_RX;
}

const char *netif_rx_get_description()
{
    return NULL;
}

const char *netif_rx_get_dep()
{
    return NULL;
}

int netif_rx_get_priority()
{
    return 0;
}

int netif_rx_get_type()
{
    return -1;
}

int netif_rx_get_period()
{
    return 100;
}

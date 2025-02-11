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

#ifndef OEAWARE_ENV_DATA_H
#define OEAWARE_ENV_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ENV_DATA_NOT_READY,
	ENV_DATA_READY
} EnvDataStatus;

// static means that the data is not changed after initialization
// if not reboot, the data is not changed
typedef struct {
    int numaNum;
    int cpuNumConfig;         // not online
    int pageSize;
    unsigned long pageMask;
    int *cpu2Node;
    int **numaDistance;       // numactl -H
} EnvStaticInfo;

typedef struct {
    /* The environmental information must be accurate,
     * otherwise it will have serious consequences for other plugins to run.
     * Therefore, a variable(dataReady) is added to indicate whether the data is complete,
     * in order to avoid the framework outputting incorrect data
     */
    int dataReady;            // ENV_DATA_NOT_READY: data not ready, ENV_DATA_READY: ready
    int cpuNumOnline;
} EnvRealTimeInfo;

#ifdef __cplusplus
}
#endif

#endif
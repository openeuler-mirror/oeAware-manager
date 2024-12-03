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
#include "pmu_uncore.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <securec.h>

static int hhaNum = 0;
static struct UncoreConfig *uncoreRxOuter = nullptr;
static struct UncoreConfig *uncorRxSccl = nullptr;
static struct UncoreConfig *uncoreRxOpsNum = nullptr;

int GetUncoreHhaNum()
{
    return hhaNum;
}

struct UncoreConfig *GetRxOuter()
{
    return uncoreRxOuter;
}

struct UncoreConfig *GetRxSccl()
{
    return uncorRxSccl;
}

struct UncoreConfig *GetRxOpsNum()
{
    return uncoreRxOpsNum;
}

static int ReadSingleUncoreEvent(const char *hhaName, struct UncoreConfig *uncoreEvent, const char *eventName)
{
    char hhaPath[MAX_PATH_LEN] = {0};

    // Read cfg
    snprintf_truncated_s(hhaPath, MAX_PATH_LEN, "%s/%s/", hhaName, eventName);
    
    strcpy_s(uncoreEvent->uncoreName, MAX_PATH_LEN, hhaPath);

    return 0;
}

static void FreeNamelist(int n, struct dirent **namelist)
{
    while (n--) {
        free(namelist[n]);
    }
    free(namelist);
}

static int HhaReadUncoreConfig(int n, struct dirent **namelist)
{
    int index = 0;

    while (index < n) {
        if (ReadSingleUncoreEvent(namelist[index]->d_name, &uncoreRxOuter[index], "rx_outer") != 0) {
            return -1;
        }
        if (ReadSingleUncoreEvent(namelist[index]->d_name, &uncorRxSccl[index], "rx_sccl") != 0) {
            return -1;
        }
        if (ReadSingleUncoreEvent(namelist[index]->d_name, &uncoreRxOpsNum[index], "rx_ops_num") != 0) {
            return -1;
        }

        index++;
    }

    return 0;
}

static int HhaScandirSelect(const struct dirent *ptr)
{
    int ret = 0;
    if (strstr(ptr->d_name, "hha") != nullptr) {
        ret = 1;
    }

    return ret;
}

int HhaUncoreConfigInit(void)
{
    int ret;
    struct dirent **namelist;

    hhaNum = scandir(DEVICE_PATH.data(), &namelist, HhaScandirSelect, alphasort);
    if (hhaNum <= 0) {
        printf("scandir failed\n");
        return -1;
    }

    uncoreRxOuter = new UncoreConfig[hhaNum];
    if (uncoreRxOuter == nullptr) {
        FreeNamelist(hhaNum, namelist);
        return -1;
    }
    uncorRxSccl = new UncoreConfig[hhaNum];
    if (uncorRxSccl == nullptr) { // free uncorRxSccl in function UncoreConfigFini
        FreeNamelist(hhaNum, namelist);
        return -1;
    }
    uncoreRxOpsNum = new UncoreConfig[hhaNum];
    if (uncoreRxOpsNum == nullptr) { // free uncoreRxOpsNum in function UncoreConfigFini
        FreeNamelist(hhaNum, namelist);
        return -1;
    }

    ret = HhaReadUncoreConfig(hhaNum, namelist);
    FreeNamelist(hhaNum, namelist);

    return ret;
}

static void UncoreConfigFree(UncoreConfig *config)
{
    if (config != nullptr) {
        delete config;
        config = nullptr;
    }
}

void UncoreConfigFini(void)
{
    UncoreConfigFree(uncoreRxOuter);
    UncoreConfigFree(uncorRxSccl);
    UncoreConfigFree(uncoreRxOpsNum);
    hhaNum = 0;
}

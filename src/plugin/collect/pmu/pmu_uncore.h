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
#ifndef __PMU_UNCORE_H__
#define __PMU_UNCORE_H__
#include <string>

const int UNCORE_NAME_SIZE = 256;
const int MAX_PATH_LEN = 256;
const std::string DEVICE_PATH = "/sys/devices/";

struct UncoreConfig {
    char uncoreName[UNCORE_NAME_SIZE];
};

const int RX_OUTER = 0;
const int RX_SCCL = 1;
const int RX_OPS_NUM = 2;
const int UNCORE_MAX = 3;

int GetUncoreHhaNum(void);
int GetUncoreL3cNum(void);
UncoreConfig *GetRxOuter(void);
UncoreConfig *GetRxSccl(void);
UncoreConfig *GetRxOpsNum(void);
UncoreConfig *GetL3cHit(void);
int HhaUncoreConfigInit(void);
int L3cUncoreConfigInit(void);
void UncoreConfigFini(void);

#endif

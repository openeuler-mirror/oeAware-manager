/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/

 #ifndef OEAWARE_DATA_PMU_L3C_DATA_H
 #define OEAWARE_DATA_PMU_L3C_DATA_H
 #include <libkperf/pmu.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 typedef struct {
     struct PmuData *pmuData;
     int len;
     uint64_t interval;
 } PmuL3cData;
 
 #ifdef __cplusplus
 }
 #endif
 #endif
 
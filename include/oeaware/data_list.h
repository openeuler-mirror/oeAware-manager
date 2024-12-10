/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef OEAWARE_DATA_LIST_H
#define OEAWARE_DATA_LIST_H
// Names of all instances.
#define OE_PMU_COUNTING_COLLECTOR    "pmu_counting_collector"
#define OE_PMU_SAMPLING_COLLECTOR    "pmu_sampling_collector"
#define OE_PMU_SPE_COLLECTOR         "pmu_spe_collector"
#define OE_PMU_UNCORE_COLLECTOR      "pmu_uncore_collector"
#define OE_DOCKER_COLLECTOR          "docker_collector"
#define OE_KERNEL_CONFIG_COLLECTOR   "kernel_config"
#define OE_THREAD_COLLECTOR          "thread_collector"
#define OE_COMMAND_COLLECTOR         "command_collector"
#define OE_UNIXBENCH_TUNE            "unixbench_tune"
#define OE_DOCKER_CPU_BURST_TUNE     "docker_cpu_burst"
#define OE_STEALTASK_TUNE            "stealtask_tune"
#define OE_SMC_TUNE                  "smc_tune"
#define OE_SEEP_TUNE                 "seep_tune"
#define OE_XCALL_TUNE                "xcall_tune"

#ifdef __cplusplus
extern "C" {
#endif
const int OK = 0;
const int FAILED = -1;

typedef struct {
    char *instanceName;
    char *topicName;
    char *params;
} CTopic;

typedef struct {
    CTopic topic;
    unsigned long long len;
    void **data;
} DataList;

typedef struct {
    int code;
    char *payload;
} Result;
#ifdef __cplusplus
}
#endif
#endif
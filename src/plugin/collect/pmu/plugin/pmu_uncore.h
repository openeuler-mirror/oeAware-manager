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

#ifdef __cplusplus
extern "C" {
#endif

#define UNCORE_NAME_SIZE   256
#define MAX_PATH_LEN       256
#define DEVICE_PATH        "/sys/devices/"

struct uncore_config {
    char uncore_name[UNCORE_NAME_SIZE];
};

enum uncore_type {
    RX_OUTER,
    RX_SCCL,
    RX_OPS_NUM,
    UNCORE_MAX,
};

int get_uncore_hha_num(void);
struct uncore_config *get_rx_outer(void);
struct uncore_config *get_rx_sccl(void);
struct uncore_config *get_rx_ops_num(void);
int hha_uncore_config_init(void);
void uncore_config_fini(void);

#ifdef __cplusplus
}
#endif

#endif

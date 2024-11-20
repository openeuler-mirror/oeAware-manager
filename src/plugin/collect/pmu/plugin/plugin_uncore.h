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
#ifndef __PLUGIN_UNCORE_H__
#define __PLUGIN_UNCORE_H__

#ifdef __cplusplus
extern "C" {
#endif

const char *uncore_get_version();
const char *uncore_get_name();
const char *uncore_get_description();
const char *uncore_get_dep();
int uncore_get_priority();
int uncore_get_type();
int uncore_get_period();
bool uncore_enable();
void uncore_disable();
const struct DataRingBuf *uncore_get_ring_buf();
void uncore_run(const struct Param *param);

#ifdef __cplusplus
}
#endif

#endif

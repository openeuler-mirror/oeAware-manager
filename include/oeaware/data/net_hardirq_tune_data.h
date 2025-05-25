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

#ifndef OE_NET_HIRQ_TUNE_DATA
#define OE_NET_HIRQ_TUNE_DATA

#ifdef __cplusplus
extern "C" {
#endif

#define OE_TOPIC_NET_HIRQ_TUNE_DEBUG_INFO "net_hardirq_tune_debug_info"
typedef struct {
    char *log;
} NetHirqTuneDebugInfo;

#ifdef __cplusplus
}
#endif
#endif

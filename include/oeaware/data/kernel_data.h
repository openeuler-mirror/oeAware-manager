/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/

#ifndef OEAWARE_DATA_KERNEL_DATA_H
#define OEAWARE_DATA_KERNEL_DATA_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct KernelDataNode {
    char *key;
    char *value;
    struct KernelDataNode *next;
} KernelDataNode;

typedef struct {
    int len;
    KernelDataNode *kernelData;
} KernelData;

KernelDataNode* CreateNode(const char *key, const char *value);
#ifdef __cplusplus
}
#endif
#endif

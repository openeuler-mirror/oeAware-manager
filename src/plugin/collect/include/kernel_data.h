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

#ifndef KERNEL_DATA_H
#define KERNEL_DATA_H

#include "base_data.h"
#include "data_register.h"
#include "serialize.h"

typedef struct KernelDataNode{
    char *key;
    char *value;
    struct KernelDataNode *next;
}KernelDataNode;

struct KernelData : public oeaware::BaseData {
    int len = 0;
    KernelDataNode *kernelData = NULL;
    static oeaware::Register<KernelData> kernelReg;
    void Serialize(oeaware::OutStream &out) const;
    void Deserialize(oeaware::InStream &in);
};

KernelDataNode* createNode(const char *key, const char *value);

#endif
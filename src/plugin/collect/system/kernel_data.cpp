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

#include <securec.h>
#include <iostream>
#include "kernel_data.h"

oeaware::Register<KernelData> KernelData::kernelReg("kernel_config");

KernelDataNode* createNode(const char *key, const char *value)
{
    KernelDataNode *node = new KernelDataNode;
    if (node == NULL) {
        std::cout << "Error: memory allocation failed" << std::endl;
        return NULL;
    }
    node->key = strdup(key);
    node->value = strdup(value);
    node->next = NULL;
    return node;
}

void KernelData::Serialize(oeaware::OutStream &out) const
{
    out << len;
    auto tmp = kernelData;
    for (int i = 0; i < len; i++) {
        std::string key(tmp->key);
        std::string value(tmp->value);
        out << key << value;
        tmp = tmp->next;
    }
}

void KernelData::Deserialize(oeaware::InStream &in)
{
    in >> len;
    for (int i = 0; i < len; i++) {
        KernelDataNode *node = new KernelDataNode;
        std::string key, value;
        in >> key >> value;

        node->key = new char[key.length() + 1];
        errno_t ret = strcpy_s(node->key, key.length() + 1, key.c_str());
        if (ret != EOK) {
            std::cout << "Deserialize failed, reason: strcpy_s failed" << std::endl;
            return;
        }
        ret = strcpy_s(node->value, value.length() + 1, value.c_str());
        if (ret != EOK) {
            std::cout << "Deserialize failed, reason: strcpy_s failed" << std::endl;
            return;
        }

        if (kernelData == NULL) {
            kernelData = node;
        } else {
            auto tmp = kernelData;
            while (tmp->next != NULL) {
                tmp = tmp->next;
            }
            tmp->next = node;
        }
    }
}
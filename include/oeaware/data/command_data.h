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

#ifndef OEAWARE_DATA_COMMAND_DATA_H
#define OEAWARE_DATA_COMMAND_DATA_H

#define ATTR_MAX_LENGTH 20
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    char *value[ATTR_MAX_LENGTH];
} CommandIter;

typedef struct {
    int itemLen;
    int attrLen;
    char *itemAttr[ATTR_MAX_LENGTH];
    CommandIter *items;
} CommandData;
#ifdef __cplusplus
}
#endif
#endif

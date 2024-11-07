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

#ifndef COMMAND_DATA_H
#define COMMAND_DATA_H

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    char *key;
    double value;
} CommandArray;

typedef struct {
    int len;
    CommandArray *mpstatArray;
} MpstatData;

typedef struct {
    int len;
    CommandArray *iostatArray;
} IostatData;

typedef struct {
    int len;
    CommandArray *vmstatArray;
} VmstatData;
#ifdef __cplusplus
}
#endif
#endif

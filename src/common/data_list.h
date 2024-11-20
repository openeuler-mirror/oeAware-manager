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
#ifndef COMMON_DATA_LIST_H
#define COMMON_DATA_LIST_H
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

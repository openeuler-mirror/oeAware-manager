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
#ifndef SDK_OE_CLIENT_H
#define SDK_OE_CLIENT_H
#include "oeaware/data_list.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef int(*Callback)(const DataList *);
int OeInit();
int OeSubscribe(const CTopic *topic, Callback callback);
int OeUnsubscribe(const CTopic *topic);
int OePublish(const DataList *dataList);
void OeClose();
#ifdef __cplusplus
}
#endif

#endif

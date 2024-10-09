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
#ifndef __PMU_NAPI_GRO_RECEIVE_ENTRY_H__
#define __PMU_NAPI_GRO_RECEIVE_ENTRY_H__

#ifdef __cplusplus
extern "C" {
#endif

const char *NapiGroRecEntryGetVer();
const char *NapiGroRecEntryGetName();
const char *NapiGroRecEntryGetDes();
const char *NapiGroRecEntryGetDep();
int NapiGroRecEntryGetPriority();
int NapiGroRecEntryGetType();
int NapiGroRecEntryGetPeriod();
bool NapiGroRecEntryEnable();
void NapiGroRecEntryDisable();
const struct DataRingBuf *NapiGroRecEntryGetBuf();
void NapiGroRecEntryRun(const struct Param *param);

#ifdef __cplusplus
}
#endif

#endif

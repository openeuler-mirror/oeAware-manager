/*
 * Copyright (c) 2024-2024 Huawei Technologies Co., Ltd. All rights reserved.
 *
 * numafast is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef PMU_COUNTING_DATA_H
#define PMU_COUNTING_DATA_H

#include "base_data.h"
#include "data_register.h"
#include "pmu.h"
#include "serialize.h"

struct PmuCountingData : public oeaware::BaseData {
    struct PmuData *pmuData = NULL;
    int len;
    static oeaware::Register<PmuCountingData> pmuCountingReg;
    void Serialize(oeaware::OutStream &out) const;
    void Deserialize(oeaware::InStream &in);
};
#endif
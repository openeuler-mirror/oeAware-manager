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
#ifndef HUGE_DETECT_H
#define HUGE_DETECT_H
#include <cstdint>
#include <string>
#include <vector>
#include "topic.h"
#include "pmu_counting_data.h"

struct TlbInfo {
    /* data */
    uint64_t l1iTlbRefill = 0;
    uint64_t l1iTlb = 0;
    uint64_t l2iTlbRefill = 0;
    uint64_t l2iTlb = 0;
    uint64_t l1dTlbRefill = 0;
    uint64_t l1dTlb = 0;
    uint64_t l2dTlbRefill = 0;
    uint64_t l2dTlb = 0;
};

struct TlbMiss {
    double l1iTlbMiss;
    double l1dTlbMiss;
    double l2iTlbMiss;
    double l2dTlbMiss;
};

class HugeDetect {
public:
    void Init(std::vector<Topic> &topics);
    void UpdateData(const std::string &topicName, PmuCountingData *data);
    void Cal();
private:
    TlbInfo tlbInfo;
    TlbMiss tlbMiss;
};

#endif
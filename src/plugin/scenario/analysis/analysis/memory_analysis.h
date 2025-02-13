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
#include <oeaware/topic.h>
#include "analysis_data.h"
#include "pmu_counting_data.h"
#include "analysis_compose.h"

namespace oeaware {
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

class MemoryAnalysis : public AnalysisCompose {
public:
    void Init() override;
    void UpdateData(const std::string &topicName, void *data) override;
    void Analysis() override;
    void* GetResult() override;
    void Reset() override;
private:
    TlbInfo tlbInfo;
    MemoryAnalysisData memoryAnalysisData;
};
}
#endif
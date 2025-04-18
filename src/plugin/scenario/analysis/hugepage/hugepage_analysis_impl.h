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
#ifndef HUGEPAGE_ANALYSIS_IMPL_H
#define HUGEPAGE_ANALYSIS_IMPL_H
#include <cstdint>
#include <string>
#include <vector>
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
    double L1dTlbMiss()
    {
        return l1dTlb == 0 ? 0 : 1.0 * l1dTlbRefill / l1dTlb;
    }
    double L1iTlbMiss()
    {
        return l1iTlb == 0 ? 0 : 1.0 * l1iTlbRefill / l1iTlb;
    }
    double L2dTlbMiss()
    {
        return l2dTlb == 0 ? 0 : 1.0 * l2dTlbRefill / l2dTlb;
    }
    double L2iTlbMiss()
    {
        return l2iTlb == 0 ? 0 : 1.0 * l2iTlbRefill / l2iTlb;
    }
    bool IsHighMiss(double threshold1, double threshold2)
    {
        return L1dTlbMiss() * 100 >= threshold1 || L1iTlbMiss() * 100 >= threshold1 ||
            L2dTlbMiss() * 100 >= threshold2 || L2iTlbMiss() * 100 >= threshold2;
    }
};

class HugepageAnalysisImpl : public AnalysisCompose {
public:
    HugepageAnalysisImpl() { }
    HugepageAnalysisImpl(double threshold1, double threshold2) : threshold1(threshold1), threshold2(threshold2) { }
    void Init() override;
    void UpdateData(const std::string &topicName, void *data) override;
    void Analysis() override;
    void* GetResult() override;
    void Reset() override;
    double threshold1 = 5;
    double threshold2 = 10;
private:
    TlbInfo tlbInfo;
};
}
#endif
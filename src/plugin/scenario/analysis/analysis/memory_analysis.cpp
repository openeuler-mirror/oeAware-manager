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
#include "memory_analysis.h"
#include <oeaware/data_list.h>
#include <securec.h>
#include "data_register.h"

namespace oeaware {
void MemoryAnalysis::Init()
{
    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l1d_tlb", ""});
    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l1d_tlb_refill", ""});
    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l1i_tlb", ""});
    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l1i_tlb_refill", ""});
    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l2d_tlb", ""});
    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l2d_tlb_refill", ""});
    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l2i_tlb", ""});
    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l2i_tlb_refill", ""});
}

void MemoryAnalysis::UpdateData(const std::string &topicName, void *data)
{
    auto countingData = static_cast<PmuCountingData*>(data);
    for (int i = 0; i < countingData->len; ++i) {
        uint64_t count = countingData->pmuData[i].count;
        if (topicName == "l1d_tlb") {
            tlbInfo.l1dTlb += count;
        } else if (topicName == "l1d_tlb_refill") {
            tlbInfo.l1dTlbRefill += count;
        } else if (topicName == "l1i_tlb") {
            tlbInfo.l1iTlb += count;
        } else if (topicName == "l1i_tlb_refill") {
            tlbInfo.l1iTlbRefill += count;
        } else if (topicName == "l2d_tlb") {
            tlbInfo.l2dTlb += count;
        } else if (topicName == "l2d_tlb_refill") {
            tlbInfo.l2dTlbRefill += count;
        } else if (topicName == "l2i_tlb") {
            tlbInfo.l2iTlb += count;
        } else if (topicName == "l2i_tlb_refill") {
            tlbInfo.l2iTlbRefill += count;
        }
    }
}

void MemoryAnalysis::Analysis()
{
    TlbMiss &tlbMiss = memoryAnalysisData.tlbMiss;
    tlbMiss.l1dTlbMiss = tlbInfo.l1dTlb == 0 ? 0 : 1.0 * tlbInfo.l1dTlbRefill / tlbInfo.l1dTlb;
    tlbMiss.l1iTlbMiss = tlbInfo.l1iTlb == 0 ? 0 : 1.0 * tlbInfo.l1iTlbRefill / tlbInfo.l1iTlb;
    tlbMiss.l2dTlbMiss = tlbInfo.l2dTlb == 0 ? 0 : 1.0 * tlbInfo.l2dTlbRefill / tlbInfo.l2dTlb;
    tlbMiss.l2iTlbMiss = tlbInfo.l2iTlb == 0 ? 0 : 1.0 * tlbInfo.l2iTlbRefill / tlbInfo.l2iTlb;
}

void* MemoryAnalysis::GetResult()
{
    return &memoryAnalysisData;
}

void MemoryAnalysis::Reset()
{
    memset_s(&tlbInfo, sizeof(tlbInfo), 0, sizeof(tlbInfo));
}
}

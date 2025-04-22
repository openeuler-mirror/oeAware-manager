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
#include "hugepage_analysis_impl.h"
#include <oeaware/data_list.h>
#include <securec.h>
#include "data_register.h"
#include "analysis_utils.h"

namespace oeaware {
void HugepageAnalysisImpl::Init()
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

void HugepageAnalysisImpl::UpdateData(const std::string &topicName, void *data)
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

void HugepageAnalysisImpl::Analysis()
{
    std::vector<int> type;
    std::vector<std::vector<std::string>> metrics;
    type.emplace_back(DATA_TYPE_MEMORY);
    metrics.emplace_back(std::vector<std::string>{"l1dtlb_miss", std::to_string(tlbInfo.L1dTlbMiss() * 100) + "%",
        (tlbInfo.L1dTlbMiss() * 100 > threshold1 ? "high" : "low")});
    type.emplace_back(DATA_TYPE_MEMORY);
    metrics.emplace_back(std::vector<std::string>{"l1itlb_miss", std::to_string(tlbInfo.L1iTlbMiss() * 100) + "%",
        (tlbInfo.L1iTlbMiss() * 100 > threshold1 ? "high" : "low")});
    type.emplace_back(DATA_TYPE_MEMORY);
    metrics.emplace_back(std::vector<std::string>{"l2dtlb_miss", std::to_string(tlbInfo.L2dTlbMiss() * 100) + "%",
        (tlbInfo.L2dTlbMiss() * 100 > threshold2 ? "high" : "low")});
    type.emplace_back(DATA_TYPE_MEMORY);
    metrics.emplace_back(std::vector<std::string>{"l2itlb_miss", std::to_string(tlbInfo.L2iTlbMiss() * 100) + "%",
        (tlbInfo.L2iTlbMiss() * 100 > threshold2 ? "high" : "low")});
    std::string conclusion;
    std::vector<std::string> suggestionItem;
    if (tlbInfo.IsHighMiss(threshold1, threshold2)) {
        conclusion = "The tlbmiss is too high.";
        suggestionItem.emplace_back("use huge page");
        suggestionItem.emplace_back("oeawarectl -e transparent_hugepage_tune");
        suggestionItem.emplace_back("reduce the number of tlb items and reduce the missing rate");
    }
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
}

void* HugepageAnalysisImpl::GetResult()
{
    return &analysisResultItem;
}

void HugepageAnalysisImpl::Reset()
{
    memset_s(&tlbInfo, sizeof(tlbInfo), 0, sizeof(tlbInfo));
}
}

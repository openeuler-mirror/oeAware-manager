#include "hugepage_analysis.h"
#include "hugepage_analysis.h"
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

#include "oeaware/utils.h"
#include "oeaware/data/pmu_plugin.h"
#include "oeaware/data/pmu_counting_data.h"
#include "oeaware/data/pmu_sampling_data.h"
#include "oeaware/data/pmu_spe_data.h"
#include "hugepage_analysis.h"

namespace oeaware {
const int MS_PER_SEC = 1000;                // 1000 : ms per sec

HugePageAnalysis::HugePageAnalysis()
{
	name = OE_HUGEPAGE_ANALYSIS;
	period = ANALYSIS_TIME_PERIOD;
	priority = 1;
	type = SCENARIO;
	version = "1.0.0";
	for (auto &topic : topicStrs) {
		supportTopics.emplace_back(Topic{name, topic, ""});
	}
	subscribeTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l1d_tlb", ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l1d_tlb_refill", ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l1i_tlb", ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l1i_tlb_refill", ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l2d_tlb", ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l2d_tlb_refill", ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l2i_tlb", ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "l2i_tlb_refill", ""});
}

Result HugePageAnalysis::Enable(const std::string &param)
{
	(void)param;
	return Result(OK);
}

void HugePageAnalysis::Disable()
{
	topicStatus.clear();
}

Result HugePageAnalysis::OpenTopic(const oeaware::Topic &topic)
{
	if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
		return Result(FAILED, "topic " + topic.topicName + " not support!");
	}
	for (auto &topic : subscribeTopics) {
		Subscribe(topic);
	}
	auto topicType = topic.GetType();
	topicStatus[topicType].isOpen = true;
	topicStatus[topicType].beginTime = std::chrono::high_resolution_clock::now();
	auto paramsMap = GetKeyValueFromString(topic.params);
	if (paramsMap.count("t")) {
		topicStatus[topicType].time = atoi(paramsMap["t"].data());
	}
	if (paramsMap.count("threshold1")) {
		topicStatus[topicType].threshold1 = atof(paramsMap["threshold1"].data());
	}
	if (paramsMap.count("threshold2")) {
		topicStatus[topicType].threshold2 = atof(paramsMap["threshold2"].data());
	}
	return Result(OK);
}

void HugePageAnalysis::CloseTopic(const oeaware::Topic &topic)
{
	for (auto &topic : subscribeTopics) {
		Unsubscribe(topic);
	}
	auto topicType = topic.GetType();
	topicStatus[topicType].isOpen = false;
	topicStatus[topicType].isPublish = false;
	topicStatus[topicType].threshold1 = THP_THRESHOLD1;
	topicStatus[topicType].threshold2 = THP_THRESHOLD2;
	memset_s(&topicStatus[topicType].tlbInfo, sizeof(topicStatus[topicType].tlbInfo), 0, sizeof(topicStatus[topicType].tlbInfo));
}

void HugePageAnalysis::UpdateData(const DataList &dataList)
{
	std::string instanceName = dataList.topic.instanceName;
	std::string topicName = dataList.topic.topicName;
	for (auto &p : topicStatus) {
		auto topicType = p.first;
		const auto &topic = Topic::GetTopicFromType(topicType);
		if (p.second.isOpen) {
			auto countingData = static_cast<PmuCountingData*>(dataList.data[0]);
			for (int i = 0; i < countingData->len; ++i) {
				uint64_t count = countingData->pmuData[i].count;
				if (topicName == "l1d_tlb") {
					p.second.tlbInfo.l1dTlb += count;
				} else if (topicName == "l1d_tlb_refill") {
					p.second.tlbInfo.l1dTlbRefill += count;
				} else if (topicName == "l1i_tlb") {
					p.second.tlbInfo.l1iTlb += count;
				} else if (topicName == "l1i_tlb_refill") {
					p.second.tlbInfo.l1iTlbRefill += count;
				} else if (topicName == "l2d_tlb") {
					p.second.tlbInfo.l2dTlb += count;
				} else if (topicName == "l2d_tlb_refill") {
					p.second.tlbInfo.l2dTlbRefill += count;
				} else if (topicName == "l2i_tlb") {
					p.second.tlbInfo.l2iTlb += count;
				} else if (topicName == "l2i_tlb_refill") {
					p.second.tlbInfo.l2iTlbRefill += count;
				}
			}
		}
	}
}

void HugePageAnalysis::PublishData(const Topic &topic)
{
	DataList dataList;
	SetDataListTopic(&dataList, topic.instanceName, topic.topicName, topic.params);
	dataList.len = 1;
	dataList.data = new void *[dataList.len];
	dataList.data[0] = &analysisResultItem;
	Publish(dataList);
}

void HugePageAnalysis::Run()
{
	auto now = std::chrono::system_clock::now();
	for (auto &item : topicStatus) {
		auto &status = item.second;
		if (status.isOpen) {
			int curTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - status.beginTime).count();
			if (!status.isPublish && curTimeMs / MS_PER_SEC >= status.time) {
				auto topicType = item.first;
				const auto &topic = Topic::GetTopicFromType(topicType);
				Analysis(topicType);
				PublishData(topic);
				status.isPublish = true;
			}
		}
	}
}

void HugePageAnalysis::Analysis(const std::string &topicType)
{
    std::vector<int> type;
    std::vector<std::vector<std::string>> metrics;
    type.emplace_back(DATA_TYPE_MEMORY);
    metrics.emplace_back(std::vector<std::string>{"l1dtlb_miss", std::to_string(topicStatus[topicType].tlbInfo.L1dTlbMiss() *
		PERCENTAGE_FACTOR) + "%", (topicStatus[topicType].tlbInfo.L1dTlbMiss() * PERCENTAGE_FACTOR >
		topicStatus[topicType].threshold1 ? "high" : "low")});
    type.emplace_back(DATA_TYPE_MEMORY);
    metrics.emplace_back(std::vector<std::string>{"l1itlb_miss", std::to_string(topicStatus[topicType].tlbInfo.L1iTlbMiss() *
		PERCENTAGE_FACTOR) + "%", (topicStatus[topicType].tlbInfo.L1iTlbMiss() * PERCENTAGE_FACTOR >
		topicStatus[topicType].threshold1 ? "high" : "low")});
    type.emplace_back(DATA_TYPE_MEMORY);
    metrics.emplace_back(std::vector<std::string>{"l2dtlb_miss", std::to_string(topicStatus[topicType].tlbInfo.L2dTlbMiss() *
		PERCENTAGE_FACTOR) + "%", (topicStatus[topicType].tlbInfo.L2dTlbMiss() * PERCENTAGE_FACTOR >
		topicStatus[topicType].threshold2 ? "high" : "low")});
    type.emplace_back(DATA_TYPE_MEMORY);
    metrics.emplace_back(std::vector<std::string>{"l2itlb_miss", std::to_string(topicStatus[topicType].tlbInfo.L2iTlbMiss() *
		PERCENTAGE_FACTOR) + "%", (topicStatus[topicType].tlbInfo.L2iTlbMiss() * PERCENTAGE_FACTOR >
		topicStatus[topicType].threshold2 ? "high" : "low")});
    std::string conclusion;
    std::vector<std::string> suggestionItem;
    if (topicStatus[topicType].tlbInfo.IsHighMiss(topicStatus[topicType].threshold1, topicStatus[topicType].threshold2)) {
        conclusion = "The tlbmiss is too high.";
        suggestionItem.emplace_back("use huge page");
        suggestionItem.emplace_back("oeawarectl -e transparent_hugepage_tune");
        suggestionItem.emplace_back("reduce the number of tlb items and reduce the missing rate");
    } else {
		conclusion = "The tlbmiss is low, donot need to enable transparent_hugepage_tune.";
	}
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
}
}
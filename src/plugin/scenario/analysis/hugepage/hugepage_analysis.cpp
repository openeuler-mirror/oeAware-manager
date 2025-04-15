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
#include "hugepage_analysis.h"
#include <string>
#include <iostream>
#include "oeaware/data/analysis_data.h"
#include "oeaware/utils.h"
#include "analysis_utils.h"
 /* oeaware manager interface */
#include "oeaware/interface.h"
/* dependent external plugin interfaces */
#include "oeaware/data/pmu_plugin.h"
#include "oeaware/data/pmu_counting_data.h"
#include "oeaware/data/pmu_sampling_data.h"
#include "oeaware/data/pmu_spe_data.h"
/* external plugin dependent interfaces */

/* internal data processing interface */

namespace oeaware {
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
}

void HugePageAnalysis::InitAnalysisCompose()
{
	oeaware::Topic memTopic{name, "hugepage", ""};
	topicImpl["hugepage"] = new HugepageAnalysisImpl(threshold1, threshold2);
}

Result HugePageAnalysis::Enable(const std::string &param)
{
	auto params_map = GetKeyValueFromString(param);
	if (params_map.count("t")) {
		time = atoi(params_map["t"].data());
	}
	if (params_map.count("threshold1")) {
		threshold1 = atof(params_map["threshold1"].data());
	}
	if (params_map.count("threshold2")) {
		threshold2 = atof(params_map["threshold2"].data());
	}
	InitAnalysisCompose();
	return Result(OK);
}

void HugePageAnalysis::Disable()
{
	for (const auto &topic : subscribeTopics) {
		Unsubscribe(topic);
	}
}

Result HugePageAnalysis::OpenTopic(const oeaware::Topic &topic)
{
	if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
		return Result(FAILED, "topic " + topic.topicName + " not support!");
	}
	auto topicType = topic.GetType();
	topicImpl[topic.topicName]->Init();
	topicStatus[topicType].open = true;
	for (auto &topic : topicImpl[topic.topicName]->subTopics) {
		Subscribe(topic);
	}
	return Result(OK);
}

void HugePageAnalysis::CloseTopic(const oeaware::Topic &topic)
{
	auto topicType = topic.GetType();
	topicStatus[topicType].open = false;
	for (auto &topic : topicImpl[topic.topicName]->subTopics) {
		Unsubscribe(topic);
	}
}

void HugePageAnalysis::UpdateData(const DataList &dataList)
{
	std::string instanceName = dataList.topic.instanceName;
	std::string topicName = dataList.topic.topicName;
	for (auto &p : topicStatus) {
		auto topicType = p.first;
		const auto &topic = Topic::GetTopicFromType(topicType);
		if (p.second.open) {
			topicImpl[topic.topicName]->UpdateData(topicName, dataList.data[0]);
		}
	}
}

void HugePageAnalysis::PublishData(const Topic &topic)
{
	DataList dataList;
	SetDataListTopic(&dataList, topic.instanceName, topic.topicName, topic.params);
	dataList.len = 1;
	dataList.data = new void *[dataList.len];
	dataList.data[0] = topicImpl[topic.topicName]->GetResult();
	Publish(dataList);
}

void HugePageAnalysis::Run()
{
	curTime++;
	if (curTime == time) {
		for (auto &item : topicStatus) {
			auto &status = item.second;
			auto topicType = item.first;
			const auto &topic = Topic::GetTopicFromType(topicType);
			if (status.open) {
				topicImpl[topic.topicName]->Analysis();
				PublishData(topic);
				topicImpl[topic.topicName]->Reset();
			}
		}
		curTime = 0;
	}
}
}
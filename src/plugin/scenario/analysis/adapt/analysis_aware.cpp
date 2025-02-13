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
#include "analysis_aware.h"
#include <string>
#include <iostream>
#include "oeaware/data/analysis_data.h"
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
AnalysisAware::AnalysisAware()
{
	name = "analysis_aware";
	period = analysis.GetPeriod();
	priority = 1;
	type = 0;
	version = "1.0.0";
	supportTopics.emplace_back(Topic{ "analysis_aware", "analysis_report", "" });
	description += "[introduction] \n";
	description += "               analyze system runtime characteristics and recommend tune plugins\n";
	description += "[version] \n";
	description += "         " + version + "\n";
	description += "[instance name]\n";
	description += "                 " + name + "\n";
	description += "[instance period]\n";
	description += "                 " + std::to_string(period) + " ms\n";
	description += "[provide topics]\n";
	description += "                 topicName:analysis_report, params:\"\", usage:get realtime analysis report\n";
	description += "                 topicName:analysis_report, params:quit, usage:get summary analysis report\n";
	description += "[running require arch]\n";
	description += "                      aarch64 only\n";
	description += "[running require environment]\n";
	description += "                 physical machine(not qemu), kernel version(4.19, 5.10, 6.6)\n";
	description += "[running require topics]\n";
	description += "                        1.pmu_spe_collector::spe\n";
	description += "                        2.pmu_counting_collector::net:netif_rx\n";
	description += "                        3.pmu_sampling_collector::cycles\n";
	description += "                        4.pmu_sampling_collector::skb:skb_copy_datagram_iovec\n";
	description += "                        5.pmu_sampling_collector::net:napi_gro_receive_entry\n";
	description += "[usage] \n";
	description += "        example:You can use `oeawarectl analysis -t 10` to get 10s report\n";
	for (auto &topic : topicStrs) {
		supportTopics.emplace_back(Topic{name, topic, ""});
	}
}

void AnalysisAware::InitAnalysisCompose()
{
	oeaware::Topic memTopic{name, MEMORY_ANALYSIS, ""};
	analysisComposes[memTopic.GetType()] = new MemoryAnalysis();
}

Result AnalysisAware::Enable(const std::string &param)
{
	(void)param;
	InitAnalysisCompose();
	return Result(OK);
}

void AnalysisAware::Disable()
{
	for (const auto &topic : subscribeTopics) {
		Unsubscribe(topic);
	}
}

bool AnalysisAware::TopicStatus::Init(const oeaware::Topic &topic)
{
	open = false;
	// check the legality of a single topic
	if (topic.topicName == "analysis_report") {
		if (topic.params != "" && topic.params != "quit") {
			return false;
		}
		return true;
	}
	return false;
}

Result AnalysisAware::OpenTopic(const oeaware::Topic &topic)
{
	if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
		return Result(FAILED, "topic " + topic.topicName + " not support!");
	}
	auto topicType = topic.GetType();
	analysisComposes[topicType]->Init();
	topicStatus[topicType].open = true;
	for (auto &topic : analysisComposes[topicType]->subTopics) {
		Subscribe(topic);
	}
	return Result(OK);
}

void AnalysisAware::CloseTopic(const oeaware::Topic &topic)
{
	auto topicType = topic.GetType();
	topicStatus[topicType].open = false;
	for (auto &topic : analysisComposes[topicType]->subTopics) {
		Unsubscribe(topic);
	}
}

void AnalysisAware::UpdateData(const DataList &dataList)
{
	std::string instanceName = dataList.topic.instanceName;
	std::string topicName = dataList.topic.topicName;
	for (auto &p : topicStatus) {
		if (p.second.open) {
			analysisComposes[p.first]->UpdateData(topicName, dataList.data[0]);
		}
	}
}

void AnalysisAware::PublishData()
{
	for (auto &item : topicStatus) {
		auto topicType = item.first;
		const auto &topic = Topic::GetTopicFromType(topicType);
		auto &status = item.second;
		if (status.open) {
			DataList dataList;
			dataList.topic.instanceName = new char[name.size() + 1];
			strcpy_s(dataList.topic.instanceName, name.size() + 1, name.data());
			dataList.topic.topicName = new char[topic.topicName.size() + 1];
			strcpy_s(dataList.topic.topicName, topic.topicName.size() + 1, topic.topicName.data());
			dataList.topic.params = new char[topic.params.size() + 1];
			strcpy_s(dataList.topic.params, topic.params.size() + 1, topic.params.data());
			dataList.len = 1;
			dataList.data = new void *[dataList.len];
			dataList.data[0] = analysisComposes[topicType]->GetResult();
			Publish(dataList);
		}
	}
}

void AnalysisAware::Analyze()
{
	for (auto &item : topicStatus) {
		auto &status = item.second;
		if (status.open) {
			analysisComposes[item.first]->Analysis();
		}
	}
}

void AnalysisAware::Reset()
{
	for (auto &item : topicStatus) {
		auto &status = item.second;
		if (status.open) {
			analysisComposes[item.first]->Reset();
		}
	}
}

void AnalysisAware::Run()
{
	bool isSummary = false;
	for (auto &item : topicStatus) {
		if (!item.second.open) {
			continue;
		}
		const Topic &topic = Topic::GetTopicFromType(item.first);
		if (topic.params == "quit") {
			isSummary = true;
		}
	}
	Analyze();
	PublishData();
	Reset();
}

extern "C" void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<AnalysisAware>());
}
}
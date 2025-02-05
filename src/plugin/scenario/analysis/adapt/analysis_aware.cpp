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
}

Result AnalysisAware::Enable(const std::string &param)
{
	(void)param;
	subscribeTopics.emplace_back(Topic{ "pmu_spe_collector", "spe", "" });
	subscribeTopics.emplace_back(Topic{ "pmu_counting_collector", "net:netif_rx", "" });
	subscribeTopics.emplace_back(Topic{ "pmu_sampling_collector", "cycles", "" });
	subscribeTopics.emplace_back(Topic{ "pmu_sampling_collector", "skb:skb_copy_datagram_iovec", "" });
	subscribeTopics.emplace_back(Topic{ "pmu_sampling_collector", "net:napi_gro_receive_entry", "" });
	for (const auto &topic : subscribeTopics) {
		Subscribe(topic);
	}
	hugeDetect.Init(subscribeTopics);
	analysis.Init();
	return Result(OK);
}

void AnalysisAware::Disable()
{
	for (const auto &topic : subscribeTopics) {
		Unsubscribe(topic);
	}
	analysis.Exit();
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
	// If the topic contains complex parameters in the future,
	// add a check here to verify whether multiple topic combinations are valid
	if (!topicStatus[topic.GetType()].Init(topic)) {
		return Result(FAILED, "topic " + topic.topicName + " not support!");
	}
	topicStatus[topic.GetType()].open = true;
	return Result(OK);
}

void AnalysisAware::CloseTopic(const oeaware::Topic &topic)
{
	topicStatus[topic.GetType()].open = false;
}

void AnalysisAware::UpdateData(const DataList &dataList)
{
	std::string topicName = dataList.topic.topicName;
	if (topicName == "spe") {
		PmuSpeData *dataTmp = static_cast<PmuSpeData *>(dataList.data[0]);
		analysis.UpdatePmu(topicName, dataTmp->len, dataTmp->pmuData, 0);
	} else if (topicName == "net:netif_rx") {
		PmuCountingData *dataTmp = static_cast<PmuCountingData *>(dataList.data[0]);
		analysis.UpdatePmu(topicName, dataTmp->len, dataTmp->pmuData, 0);
	} else if (topicName == "cycles") {
		PmuSamplingData *dataTmp = static_cast<PmuSamplingData *>(dataList.data[0]);
		analysis.UpdatePmu(topicName, dataTmp->len, dataTmp->pmuData, dataTmp->interval);
	} else if (topicName == "skb:skb_copy_datagram_iovec") {
		PmuSamplingData *dataTmp = static_cast<PmuSamplingData *>(dataList.data[0]);
		analysis.UpdatePmu(topicName, dataTmp->len, dataTmp->pmuData, 0);
	} else if (topicName == "net:napi_gro_receive_entry") {
		PmuSamplingData *dataTmp = static_cast<PmuSamplingData *>(dataList.data[0]);
		analysis.UpdatePmu(topicName, dataTmp->len, dataTmp->pmuData, 0);
	} else {
		hugeDetect.UpdateData(topicName, static_cast<PmuCountingData *>(dataList.data[0]));
	}
}

void AnalysisAware::PublishData()
{
	for (auto &item : topicStatus) {
		const auto &topic = Topic::GetTopicFromType(item.first);
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
			AnalysisReport *report = new AnalysisReport();
			bool isSummary = topic.params == "quit";
			const std::string &str = analysis.GetReport(isSummary);
			report->data = new char [str.size() + 1];
			strcpy_s(report->data, str.size() + 1, str.data());
			report->reportType = isSummary ? ANALYSIS_REPORT_SUMMARY : ANALYSIS_REPORT_REALTIME;
			dataList.data[0] = report;
			Publish(dataList);
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
	analysis.Analyze(isSummary);
	hugeDetect.Cal();
	PublishData();
	pmuData.clear();
}

extern "C" void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<AnalysisAware>());
}
}
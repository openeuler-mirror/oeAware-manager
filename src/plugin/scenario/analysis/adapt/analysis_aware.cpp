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
#include "adapt_data.h"
 /* oeaware manager interface */
#include "interface.h"
/* dependent external plugin interfaces */
#include "pmu_plugin.h"
#include "pmu_counting_data.h"
#include "pmu_sampling_data.h"
#include "pmu_spe_data.h"
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
	description = "pmu_spe_collector";
    description += "-";
    description += "pmu_counting_collector";
    description += "-";
    description += "pmu_sampling_collector";
	supportTopics.emplace_back(Topic{"analysis_aware", "analysis_aware", ""});
}

Result AnalysisAware::Enable(const std::string &param)
{
	(void)param;
	Subscribe(Topic{"pmu_spe_collector", "spe", ""});
	Subscribe(Topic{"pmu_counting_collector", "net:netif_rx", ""});
	Subscribe(Topic{"pmu_sampling_collector", "cycles", ""});
	Subscribe(Topic{"pmu_sampling_collector", "skb:skb_copy_datagram_iovec", ""});
	Subscribe(Topic{"pmu_sampling_collector", "net:napi_gro_receive_entry", ""});
	analysis.Init();
	return Result(OK);
}

void AnalysisAware::Disable()
{
	analysis.Exit();
}

Result AnalysisAware::OpenTopic(const oeaware::Topic &topic)
{
	(void)topic;
	return Result(OK);
}

void AnalysisAware::CloseTopic(const oeaware::Topic &topic)
{
	(void)topic;
}

void AnalysisAware::UpdateData(const DataList &dataList)
{
	std::string topicName = dataList.topic.topicName;
	if (topicName == "spe") {
		PmuSpeData* speData = static_cast<PmuSpeData*>(dataList.data[0]);
		if (speData->len && speData->pmuData) {
			pmuData[PMU_SPE].emplace_back(*(speData->pmuData));
		}
	} else if (topicName == "net:netif_rx") {
		PmuCountingData* countingData = static_cast<PmuCountingData*>(dataList.data[0]);
		if (countingData->len && countingData->pmuData) {
			pmuData[PMU_NETIF_RX].emplace_back(*(countingData->pmuData));
		}
	} else if (topicName == "cycles") {
		PmuSamplingData* samplingData = static_cast<PmuSamplingData*>(dataList.data[0]);
		if (samplingData->len && samplingData->pmuData) {
			pmuData[PMU_CYCLES_SAMPLING].emplace_back(*(samplingData->pmuData));
		}
	} else if (topicName == "skb:skb_copy_datagram_iovec") {
		PmuSamplingData* samplingData = static_cast<PmuSamplingData*>(dataList.data[0]);
		if (samplingData->len && samplingData->pmuData) {
			pmuData[PMU_SKB_COPY_DATEGRAM_IOVEC].emplace_back(*(samplingData->pmuData));
		}
	} else if (topicName == "net:napi_gro_receive_entry") {
		PmuSamplingData* samplingData = static_cast<PmuSamplingData*>(dataList.data[0]);
		if (samplingData->len && samplingData->pmuData) {
			pmuData[PMU_NAPI_GRO_REC_ENTRY].emplace_back(*(samplingData->pmuData));
		}
	}
}

void AnalysisAware::Run()
{
    UpdatePmu();
    analysis.Analyze();
	analysisData = analysis.GetAnalysisData();
	AdaptData* data = new AdaptData();
	data->len = analysisData.size();
	data->data = new char*[data->len];
	for (size_t i = 0; i < analysisData.size(); ++i) {
		data->data[i] = new char[analysisData[i].size() + 1];
		strcpy_s(data->data[i], analysisData[i].size() + 1, analysisData[i].data());
	}
	DataList dataList;
	dataList.topic.instanceName = new char[name.size() + 1];
	strcpy_s(dataList.topic.instanceName, name.size() + 1, name.data());
	dataList.topic.topicName = new char[name.size() + 1];
	strcpy_s(dataList.topic.topicName, name.size() + 1, name.data());
	dataList.topic.params = new char[1];
	dataList.topic.params[0] = 0;
	dataList.len = 1;
	dataList.data = new void* [1];
	dataList.data[0] = data;
	Publish(dataList);
	pmuData.clear();
}

void AnalysisAware::UpdatePmu()
{
	for (const auto& itMap : pmuData) {
		for (const auto& itVec : itMap.second) {
			analysis.UpdatePmu(itMap.first, 1, (PmuData *)&itVec);
		}
	}
}

extern "C" void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<AnalysisAware>());
}
}

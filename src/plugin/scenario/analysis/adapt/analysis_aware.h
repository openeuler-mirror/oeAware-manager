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
#ifndef ANALYSIS_AWARE_H
#define ANALYSIS_AWARE_H
#include "oeaware/interface.h"
#include "analysis.h"
#include "analysis_compose.h"
#include "libkperf/pmu.h"
#include "memory_analysis.h"

namespace oeaware {
class AnalysisAware : public Interface {
public:
	AnalysisAware();
	~AnalysisAware() override = default;
	Result OpenTopic(const oeaware::Topic &topic) override;
	void CloseTopic(const oeaware::Topic &topic) override;
	void UpdateData(const DataList &dataList) override;
	Result Enable(const std::string &param) override;
	void Disable() override;
	void Run() override;
	struct TopicStatus {
		bool Init(const oeaware::Topic &topic);
		bool open = false;
	};
private:
	std::vector<Topic> subscribeTopics;
	void PublishData();
	void InitAnalysisCompose();
	void Analyze();
	void Reset();
private:
	Analysis analysis;
	std::unordered_map<std::string, AnalysisCompose*> analysisComposes;
	std::vector<std::string> topicStrs{MEMORY_ANALYSIS};
	std::vector<std::string> analysisData;
	std::unordered_map<std::string, PmuData *> pmuData;
	std::unordered_map<std::string, TopicStatus> topicStatus;
};
}
#endif


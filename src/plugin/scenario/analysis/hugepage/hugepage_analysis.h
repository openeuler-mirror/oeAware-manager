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
#ifndef HUGEPAGE_ANALYSIS_H
#define HUGEPAGE_ANALYSIS_H
#include "oeaware/interface.h"
#include "analysis.h"
#include "analysis_compose.h"
#include "hugepage_analysis_impl.h"

namespace oeaware {
class HugePageAnalysis : public Interface {
public:
	HugePageAnalysis();
	~HugePageAnalysis() override = default;
	Result OpenTopic(const oeaware::Topic &topic) override;
	void CloseTopic(const oeaware::Topic &topic) override;
	void UpdateData(const DataList &dataList) override;
	Result Enable(const std::string &param) override;
	void Disable() override;
	void Run() override;
	struct TopicStatus {
		bool open = false;
	};
private:
	std::vector<Topic> subscribeTopics;
	void PublishData(const Topic &topic);
	void InitAnalysisCompose();
private:
	std::unordered_map<std::string, AnalysisCompose*> topicImpl;
	std::vector<std::string> topicStrs{"hugepage"};
	std::vector<std::string> analysisData;
	std::unordered_map<std::string, PmuData *> pmuData;
	std::unordered_map<std::string, TopicStatus> topicStatus;
	int time = 10;
	int curTime = 0;
	double threshold1 = 5;
    double threshold2 = 10;
};
}
#endif


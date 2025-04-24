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
#include "analysis_utils.h"

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
private:
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
			return L1dTlbMiss() * PERCENTAGE_FACTOR >= threshold1 ||
				L1iTlbMiss() * PERCENTAGE_FACTOR >= threshold1 ||
				L2dTlbMiss() * PERCENTAGE_FACTOR >= threshold2 ||
				L2iTlbMiss() * PERCENTAGE_FACTOR >= threshold2;
		}
	};
	struct TopicStatus {
		bool isOpen = false;
		bool isPublish = false;
		int curTime = 0;
		int time;
		double threshold1 = THP_THRESHOLD1;
		double threshold2 = THP_THRESHOLD2;
		TlbInfo tlbInfo;
	};
	void PublishData(const Topic &topic);
	void Analysis(const std::string &topicType);
	std::vector<std::string> topicStrs{"hugepage"};
	std::unordered_map<std::string, TopicStatus> topicStatus;
	std::vector<Topic> subscribeTopics;
	AnalysisResultItem analysisResultItem;
};
}
#endif


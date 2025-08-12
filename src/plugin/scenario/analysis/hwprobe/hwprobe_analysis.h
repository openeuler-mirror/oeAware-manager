
/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef HWPROBE_ANALYSIS_H
#define HWPROBE_ANALYSIS_H
#include "oeaware/interface.h"
#include "analysis.h"
#include "analysis_utils.h"
#include <vector>
#include <string>
#include <map>
#include <asm/hwprobe.h>

namespace oeaware {
class HwprobeAnalysis : public Interface {
public:
	HwprobeAnalysis();
	~HwprobeAnalysis() override = default;
	Result OpenTopic(const oeaware::Topic &topic) override;
	void CloseTopic(const oeaware::Topic &topic) override;
	void UpdateData(const DataList &dataList) override;
	Result Enable(const std::string &param) override;
	void Disable() override;
	void Run() override;
private:
	struct TopicStatus {
		bool isOpen = false;
		bool isPublish = false;
		int time = 1;
		std::chrono::time_point<std::chrono::high_resolution_clock> beginTime;
	};
	void PublishData(const Topic &topic);
	void Analysis(const std::string &topicType);
	std::vector<std::string> topicStrs{"hwprobe_analysis"};
	std::unordered_map<std::string, TopicStatus> topicStatus;
	std::vector<Topic> subscribeTopics = {};
	AnalysisResultItem analysisResultItem = {};
	const int MS_PER_SEC = 1000;

    struct DynamicKey {
        int64_t key;
        std::string macro_name;
    };
    
    bool CheckHwprobeHeader();
    std::vector<DynamicKey> ParseHwprobeKeys();
    bool ProbeHwInfo(std::vector<DynamicKey>& keys, std::vector<riscv_hwprobe>& pairs);
    std::string ParseHwprobeNote(int64_t key, uint64_t value);
};
}
#endif

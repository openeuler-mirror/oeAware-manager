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
#ifndef NET_HIRQ_ANALYSIS_H
#define NET_HIRQ_ANALYSIS_H
#include "oeaware/interface.h"
#include "oeaware/data/pmu_sampling_data.h"
#include "oeaware/data/analysis_data.h"

namespace oeaware {
struct AnalysisRst {
    std::string dev;
    bool shouldTune = false;
    float peak = 0.0;
    float average = 0.0;
};
class NetHirqAnalysis : public Interface {
public:
	NetHirqAnalysis();
	~NetHirqAnalysis() override = default;
	Result OpenTopic(const oeaware::Topic &topic) override;
	void CloseTopic(const oeaware::Topic &topic) override;
	void UpdateData(const DataList &dataList) override;
	Result Enable(const std::string &param) override;
	void Disable() override;
    void Run() override;

private:
    struct NetRx {
        uint64_t interval = 0;     // use to calculate rxAvg
        uint64_t rxSum = 0;        // receive how many queue trace data
        float    rxAvg = 0.0;
    };
    struct TopicCtl {
        std::string topicName;
        std::string openParams;   // assuming there is only time parameter
        bool isOpen = false;
        int analysisTime = 0;
        bool hasPublished = false;
        std::chrono::time_point<std::chrono::high_resolution_clock> beginTime;
    };
    std::vector<std::string> topicStr = { OE_NET_HIRQ_ANALYSIS };
    std::unordered_map<std::string, TopicCtl> topicCtl; // topic name to topic info
    std::vector<oeaware::Topic> subscribeTopics;
    std::unordered_map<std::string, NetRx> netRxSum;    // dev name to rx sum
    std::unordered_map<std::string, std::vector<NetRx>> netRxSumTrace;
    std::unordered_map<std::string, AnalysisRst> result; // dev name to analysis result
    AnalysisResultItem analysisResultItem = {};
    AnalysisRst GetAnalysisResult(const std::string &dev, const std::vector<NetRx> &netRxVec);
    void GenPublishData();
};
}
#endif


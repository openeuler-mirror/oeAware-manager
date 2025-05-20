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
#ifndef NUMA_ANALYSIS_H
#define NUMA_ANALYSIS_H

#include "oeaware/interface.h"
#include "analysis.h"
#include "analysis_utils.h"

namespace oeaware {

class NumaAnalysis : public Interface {
public:
    NumaAnalysis();
    ~NumaAnalysis() override = default;

    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

    struct TopicStatus {
        bool open = false;
        bool isPublish = false;
    };

private:
    void PublishData(const Topic &topic);
    void Analysis();
    void* GetResult();
    void Reset();
    bool CheckNumaBottleneck();
    void LoadConfig();
    bool IsSupportNumaSchedParal();

    std::vector<Topic> subscribeTopics;
    std::vector<Topic> subTopics;

    std::vector<std::string> topicStrs{"numa_analysis"};
    std::vector<std::string> analysisData;
    std::unordered_map<std::string, TopicStatus> topicStatus;
    std::vector<std::string> schedFeatues;
    std::string schedFeaturePath;

    int time = 10;
    std::chrono::time_point<std::chrono::high_resolution_clock> beginTime;

    int threadCreatedCount = 0;
    int threadDestroyedCount = 0;
    int timeWindowMs = 100;
    int threadThreshold = 200;

    bool isNumaBottleneck = false;
    uint64_t totalOpsNum = 0;
    uint64_t totalRxOuter = 0;
    uint64_t totalRxSccl = 0;
    double remoteAccessRatio = 0.0;

    AnalysisResultItem analysisResultItem = {};
};

}  // namespace oeaware
#endif  // NUMA_ANALYSIS_H
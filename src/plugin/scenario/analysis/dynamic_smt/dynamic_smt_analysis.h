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
#ifndef DYNAMIC_SMT_ANALYSIS_H
#define DYNAMIC_SMT_ANALYSIS_H
#include "oeaware/interface.h"
#include "analysis.h"

constexpr int PERCENTAGE_FACTOR = 100;
constexpr int THRESHOLD = 40;

namespace oeaware {
class DynamicSmtAnalysis : public Interface {
public:
    DynamicSmtAnalysis();
    ~DynamicSmtAnalysis() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
private:
    void PublishData(const Topic &topic);
    void Analysis();
    std::vector<std::string> topicStrs{"dynamic_smt"};
    std::unordered_map<std::string, bool> topicStatus;
    std::vector<Topic> subscribeTopics;
    AnalysisResultItem analysisResultItem;
    int time = 10;
    int curTime = 0;
    double threshold = THRESHOLD;
    double cpuUsage = 0;
};
}
#endif


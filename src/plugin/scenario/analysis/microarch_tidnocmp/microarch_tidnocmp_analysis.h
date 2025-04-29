/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef MICROARCH_TIDNOCMP_ANALYSIS_H
#define MICROARCH_TIDNOCMP_ANALYSIS_H
#include <set>
#include "oeaware/interface.h"
#include "analysis.h"
#include "analysis_utils.h"

namespace oeaware {

class MicroarchTidNoCmpAnalysis : public Interface {
public:
    MicroarchTidNoCmpAnalysis();

    ~MicroarchTidNoCmpAnalysis() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
private:

    void PublishData(const Topic &topic);
    void Analysis(const std::string &topicType);
    void Reset(const std::string &topicType);
    bool ParseConfig(const std::string &topicType, const std::string &param);
    std::string JoinThreadList(const std::set<std::string> &list) const;
    void UpdateThreadData(const std::string &topicType, const DataList &dataList);

private:
    struct TopicStatus {
        bool isOpen = false;
        bool isPublish = false;
        int time;
        std::chrono::time_point<std::chrono::high_resolution_clock> beginTime;
        std::set<std::string> supportCpuPartId;
        std::set<std::string> supportServiceList;
        std::set<std::string> catchedThreadList;
    };
    std::string cpuPartId;
    std::vector<Topic> subscribeTopics;
    AnalysisResultItem analysisResultItem;
    std::vector<std::string> topicStrs{"microarch_tidnocmp"};
    std::vector<std::string> analysisData;
    std::unordered_map<std::string, TopicStatus> topicStatus;
};
}  // namespace oeaware
#endif

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
#ifndef SOFT_DOMAIN_ANALYSIS_H
#define SOFT_DOMAIN_ANALYSIS_H

#include "oeaware/interface.h"
#include "oeaware/data/docker_data.h"
#include "oeaware/data/thread_info.h"
#include "oeaware/data/env_data.h"
#include "analysis_utils.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

namespace oeaware {

struct DockerAnalysisInfo {
    std::string containerId;
    std::string containerName;
    int cpuQuota = 0;              // CPU配额
    bool shouldRecommend = false; // 是否推荐进行分域调度
};

struct AnalysisResult {
    bool shouldTune = false;       // 是否需要调优
    int recommendedDockerCount = 0; // 推荐的docker数量（配额存在且小于1个numa）
    int totalDockerCount = 0;      // 总docker数量
    std::vector<DockerAnalysisInfo> recommendedDockerInfos; // 推荐的docker详细信息
};

class SoftDomainAnalysis : public Interface {
public:
    SoftDomainAnalysis();
    ~SoftDomainAnalysis() override = default;

    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    struct TopicCtl {
        std::string topicName;
        std::string openParams;
        bool isOpen = false;
        int analysisTime = 10;      // 默认分析时间10秒
        bool hasPublished = false;
        std::chrono::time_point<std::chrono::high_resolution_clock> beginTime;
    };

    void Analysis();
    void GeneratePublishData();
    int GetContainerCpuQuota(const Container &container);
    int GetContainerNumaNode(const Container &container);
    bool IsSoftDomainSupported();
    std::string GetContainerName(const std::string &containerId);

    std::vector<std::string> topicStrs{"soft_domain_analysis"};
    std::unordered_map<std::string, TopicCtl> topicCtl;
    std::vector<oeaware::Topic> subscribeTopics;
    
    // 数据存储
    std::unordered_map<std::string, Container> dockerContainers;            // containerId -> container
    AnalysisResult analysisResult;
    AnalysisResultItem analysisResultItem = {};
    
    // 环境信息
    bool envDataReady = false;
    int numaNum = 0;
    int cpuNumConfig = 0;
    int cpuCountPerNuma = 0;       // 每个NUMA节点的CPU数量
    std::vector<int> cpu2Node;
};

} // namespace oeaware

#endif // SOFT_DOMAIN_ANALYSIS_H


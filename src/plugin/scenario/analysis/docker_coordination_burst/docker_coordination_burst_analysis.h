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
#ifndef DOCKER_COORDINATION_BURST_ANALYSIS_H
#define DOCKER_COORDINATION_BURST_ANALYSIS_H
#include "oeaware/interface.h"
#include "analysis.h"
#include "env_data.h"
#include "analysis_utils.h"

constexpr double HOST_CPU_USAGE_THRESHOLD_DEFAULT = 45.0;
constexpr double DOCKER_CPU_USAGE_THRESHOLD_DEFAULT = 95.0;

namespace oeaware {

class DockerInfo {
public:
    DockerInfo() = default;
    DockerInfo(std::string name, int64_t cfsPeriod, int64_t cfsQuota, int hostCpuNum)
        : id(std::move(name)), cfsPeriodUs(cfsPeriod), cfsQuotaUs(cfsQuota), hostCpuNum(hostCpuNum)
    { }
    std::string GetId() const
    {
        return id;
    }

    void SetSoftQuota(int64_t softQuotaValue)
    {
        softQuota = softQuotaValue;
    }
    int64_t GetSoftQuota() const
    {
        return softQuota;
    }

    double GetCpuUtil() const;
    void UpdateCpuUtil(const uint64_t cpuUsage, const uint64_t samplingTimeStamp);

private:
    std::string id;
    int64_t cfsPeriodUs = 0;
    int64_t cfsQuotaUs = 0;
    int64_t softQuota = -1;
    int hostCpuNum = -1;
    uint64_t startTimestamp = 0;
    uint64_t endTimeStamp = 0;
    uint64_t startCpuUsage = 0;
    uint64_t endCpuUsage = 0;
};

class DockerCoordinationBurstAnalysis : public Interface {
public:
    DockerCoordinationBurstAnalysis();
    ~DockerCoordinationBurstAnalysis() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    void PublishData(const Topic &topic);
    void Reset(const std::string &topicType);
    void Analysis(const std::string &topicType);
    void UpdateDockerInfo(const std::string &topicType, const DataList &dataList);
    void UpdateCpuUtil(const std::string &topicType, const EnvCpuUtilParam *cpuUtilData);
    bool Init();
	bool ParseConfig(const std::string topicType, const std::string &param);
    bool IsTuneSupport();
    void HandleSuggestDocker(const std::string &topicType, int &suggestDockerNum,
                             std::string &suggestionDockers, std::vector<int> &type,
                             std::vector<std::vector<std::string>> &metrics);

private:
    struct TopicStatus {
        bool isOpen = false;
        bool isPublish = false;
        int time;
        std::chrono::time_point<std::chrono::high_resolution_clock> beginTime;
        std::unordered_map<std::string, DockerInfo> dockerList{};
        double hostCpuUsageThreshold = HOST_CPU_USAGE_THRESHOLD_DEFAULT;
        double dockerCpuUsagethreshold = DOCKER_CPU_USAGE_THRESHOLD_DEFAULT;
        double hostCpuUtil = 0.0;
        double hostCpuUtilSum = 0.0;
        uint64_t sampleCount = 0;
    };
    int hostCpuNum = -1;
    std::vector<Topic> subscribeTopics;
    AnalysisResultItem analysisResultItem = {};
    std::vector<std::string> topicStrs{"docker_coordination_burst"};
    std::vector<std::string> analysisData;
    std::unordered_map<std::string, TopicStatus> topicStatus;
};
}  // namespace oeaware
#endif

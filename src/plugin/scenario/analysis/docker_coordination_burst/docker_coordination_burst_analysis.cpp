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
#include "docker_coordination_burst_analysis.h"
#include <string>
#include <iostream>
#include <securec.h>
#include "oeaware/interface.h"
#include "oeaware/data/analysis_data.h"
#include "oeaware/utils.h"
#include "oeaware/data_list.h"
#include "analysis_utils.h"
#include "data_register.h"
#include "docker_data.h"

constexpr int PERCENTAGE_FACTOR = 100;
constexpr int MS_PER_SEC = 1000;
const std::string SCHED_SOFT_RUNTIME_RATIO_PATH =
    "/proc/sys/kernel/sched_soft_runtime_ratio";
constexpr double MIN_PERCENT = 0.0;
constexpr double MAX_PERCENT = 100.0;
constexpr double MAX_RATIO = 20.0;

namespace oeaware {
DockerCoordinationBurstAnalysis::DockerCoordinationBurstAnalysis()
{
    name = OE_DOCKER_COORDINATION_BURST_ANALYSIS;
    period = ANALYSIS_TIME_PERIOD;
    priority = 1;
    type = SCENARIO;
    version = "1.0.0";
    for (auto &topic : topicStrs) {
        supportTopics.emplace_back(Topic{name, topic, ""});
    }
    Init();
}

Result DockerCoordinationBurstAnalysis::Enable(const std::string &param)
{
    (void)param;
    return Result(OK);
}

void DockerCoordinationBurstAnalysis::Disable()
{
    AnalysisResultItemFree(&analysisResultItem);
    topicStatus.clear();
}

bool DockerCoordinationBurstAnalysis::ParseConfig(const std::string topicType, const std::string &param)
{
    auto paramsMap = GetKeyValueFromString(param);
    if (paramsMap.count("t")) {
        topicStatus[topicType].time = atoi(paramsMap["t"].data());
    }

    if (paramsMap.count("hostCpuUsageThreshold")) {
        double tmpHostCpuUsageThreshold = atof(paramsMap["hostCpuUsageThreshold"].data());
        if (tmpHostCpuUsageThreshold < MIN_PERCENT || tmpHostCpuUsageThreshold > MAX_PERCENT) {
            ERROR(logger,
                "DockerCoordinationBurstAnalysis param hostCpuUsageThreshold "
                    << tmpHostCpuUsageThreshold << " is invalid. It should be in [0, 100].");
            return false;
        } else {
            topicStatus[topicType].hostCpuUsageThreshold = tmpHostCpuUsageThreshold;
        }
    } else {
        ERROR(logger,
              "DockerCoordinationBurstAnalysis param hostCpuUsageThreshold is invalid.");
        return false;
    }
    if (paramsMap.count("dockerCpuUsageThreshold")) {
        double tmpDockerCpuUsagethreshold = atof(paramsMap["dockerCpuUsageThreshold"].data());
        if (tmpDockerCpuUsagethreshold < MIN_PERCENT || tmpDockerCpuUsagethreshold > MAX_PERCENT) {
            ERROR(logger,
                "DockerCoordinationBurstAnalysis param dockerCpuUsageThreshold "
                    << tmpDockerCpuUsagethreshold << " is invalid. It should be in [0, 100].");
            return false;
        } else {
            topicStatus[topicType].dockerCpuUsagethreshold = tmpDockerCpuUsagethreshold;
        }
    } else {
        ERROR(logger,
              "DockerCoordinationBurstAnalysis param dockerCpuUsageThreshold is invalid.");
        return false;
    }

    return true;
}

Result DockerCoordinationBurstAnalysis::OpenTopic(const oeaware::Topic &topic)
{
    if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
        return Result(FAILED, "topic " + topic.topicName + " not support!");
    }

    auto topicType = topic.GetType();
    Reset(topicType);
    if (!ParseConfig(topicType, topic.params)) {
        return Result(FAILED, "topic " + topicType + " open failed! Invalid config!");
    }

    subscribeTopics.emplace_back(oeaware::Topic{OE_ENV_INFO, "cpu_util", ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_PMU_SAMPLING_COLLECTOR, "cycles", ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_DOCKER_COLLECTOR, OE_DOCKER_COLLECTOR, ""});
    topicStatus[topicType].isOpen = true;
    topicStatus[topicType].beginTime = std::chrono::high_resolution_clock::now();
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }

    return Result(OK);
}

void DockerCoordinationBurstAnalysis::CloseTopic(const oeaware::Topic &topic)
{
    auto topicType = topic.GetType();
    topicStatus[topicType].isOpen = false;
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    Reset(topicType);
}

bool DockerCoordinationBurstAnalysis::Init()
{
    hostCpuNum = sysconf(_SC_NPROCESSORS_CONF);
    if (hostCpuNum <= 0) {
        ERROR(logger, "can not get host cpu num");
        return false;
    }
    return true;
}

void DockerCoordinationBurstAnalysis::UpdateDockerInfo(const std::string &topicType, const DataList &dataList)
{
    for (unsigned long long i = 0; i < dataList.len; i++) {
        auto *container = static_cast<Container *>(dataList.data[i]);
        auto dockerIt = topicStatus[topicType].dockerList.find(container->id);
        if (dockerIt == topicStatus[topicType].dockerList.end()) {
            DockerInfo dockerInfo(container->id, container->cfs_period_us, container->cfs_quota_us, hostCpuNum);
            dockerInfo.UpdateCpuUtil(container->cpu_usage, container->sampling_timestamp);
            dockerInfo.SetSoftQuota(container->soft_quota);
            topicStatus[topicType].dockerList.emplace(container->id, dockerInfo);
        } else {
            dockerIt->second.UpdateCpuUtil(container->cpu_usage, container->sampling_timestamp);
            dockerIt->second.SetSoftQuota(container->soft_quota);
        }
    }
}

void DockerCoordinationBurstAnalysis::UpdateCpuUtil(const std::string &topicType, const EnvCpuUtilParam *cpuUtilData)
{
    if (cpuUtilData->dataReady != ENV_DATA_READY) {
        return;
    }
    auto cpuIdle = cpuUtilData->times[cpuUtilData->cpuNumConfig][CPU_IDLE];
    auto cpuSum = cpuUtilData->times[cpuUtilData->cpuNumConfig][CPU_TIME_SUM];
    auto curCpuUtil = static_cast<double>(cpuSum - cpuIdle) / cpuSum;
    topicStatus[topicType].hostCpuUtilSum += curCpuUtil;
    topicStatus[topicType].sampleCount++;
    topicStatus[topicType].hostCpuUtil = topicStatus[topicType].hostCpuUtilSum / topicStatus[topicType].sampleCount;
}

void DockerCoordinationBurstAnalysis::UpdateData(const DataList &dataList)
{
    std::string instanceName = dataList.topic.instanceName;
    std::string topicName = dataList.topic.topicName;
    for (auto &p : topicStatus) {
        auto topicType = p.first;
        if (p.second.isOpen) {
            if (topicName == "cpu_util") {
                UpdateCpuUtil(topicType, static_cast<EnvCpuUtilParam *>(dataList.data[0]));
            }
            if (topicName == OE_DOCKER_COLLECTOR) {
                UpdateDockerInfo(topicType, dataList);
            }
        }
    }
}

void DockerCoordinationBurstAnalysis::PublishData(const Topic &topic)
{
    DataList dataList;
    SetDataListTopic(&dataList, topic.instanceName, topic.topicName, topic.params);
    dataList.len = 1;
    dataList.data = new void *[dataList.len];
    dataList.data[0] = &analysisResultItem;
    Publish(dataList, false);
}

bool DockerCoordinationBurstAnalysis::IsTuneSupport()
{
    std::ifstream inFile(SCHED_SOFT_RUNTIME_RATIO_PATH);
    return inFile.good();
}

void DockerCoordinationBurstAnalysis::HandleSuggestDocker(const std::string &topicType,
                                                          int &suggestDockerNum,
                                                          std::string &suggestionDockers,
                                                          std::vector<int> &type,
                                                          std::vector<std::vector<std::string>> &metrics)
{
    for (auto &docker : topicStatus[topicType].dockerList) {
        type.emplace_back(DATA_TYPE_CPU);
        auto dockerID = docker.second.GetId();
        auto dockerCpuUtil = docker.second.GetCpuUtil();
        auto dockerCpuUsageThreshold = topicStatus[topicType].dockerCpuUsagethreshold;

        std::string dockerShortId = dockerID.substr(0, 12);
        metrics.emplace_back(std::vector<std::string>{"docker_cpu_usage[" + dockerShortId + "]",
            std::to_string(dockerCpuUtil) + "%",
            (dockerCpuUtil > dockerCpuUsageThreshold ? "high for docker coordination burst"
                                                                            : "low for docker coordination burst")});
        if (dockerCpuUtil > dockerCpuUsageThreshold && docker.second.GetSoftQuota() == 0) {
            suggestionDockers += dockerShortId + ",";
            suggestDockerNum++;
        }
    }
    if (!suggestionDockers.empty() && suggestionDockers.back() == ',') {
        suggestionDockers.pop_back();
    }
}

void DockerCoordinationBurstAnalysis::Analysis(const std::string &topicType)
{
    std::vector<std::vector<std::string>> metrics;
    std::vector<int> type;

    double hostCpuUtil = topicStatus[topicType].hostCpuUtil * PERCENTAGE_FACTOR;
    double hostCpuUsageThreshold = topicStatus[topicType].hostCpuUsageThreshold;

    type.emplace_back(DATA_TYPE_CPU);
    metrics.emplace_back(std::vector<std::string>{"host_cpu_usage",
        std::to_string(hostCpuUtil) + "%",
        (hostCpuUtil > hostCpuUsageThreshold
                ? "high for docker coordination burst"
                : "low for docker coordination burst")});

    std::string suggestionDockers;
    int suggestDockerNum = 0;
    HandleSuggestDocker(topicType, suggestDockerNum, suggestionDockers, type, metrics);

    std::string conclusion;
    std::vector<std::string> suggestionItem;
    if (topicStatus[topicType].dockerList.empty()) {
        conclusion = "No docker found in this environment. So no need to use docker coordination burst.";
    }

    if (hostCpuUtil <= hostCpuUsageThreshold) {
        if (suggestionDockers.empty()) {
            conclusion = "The system loads is low, but the docker load is not high or the docker"
                         " coordination burst is already open, do not need to enable"
                         " docker coordination burst.";
        } else {
            if (IsTuneSupport()) {
                conclusion = "The system loads is low. The dockers which cpu loads are high "
                            "can use docker coordination burst.";
                suggestionItem.emplace_back("use docker coordination burst");
                suggestionItem.emplace_back("oeawarectl -e docker_burst -docker_id " + suggestionDockers + " -ratio " +
                                            std::to_string(static_cast<int>(MAX_RATIO)));
                suggestionItem.emplace_back("The idle cpu of the scheduling physical machine is scheduled to "
                                            "heavy load containers to improve performance.");
            } else {
                conclusion = "The system loads is low. The dockers which cpu loads are high "
                            "can use docker coordination burst. but the environment is not "
                            "support docker coordination burst tune.";
            }
        }
    } else {
        conclusion = "The system loads is high, and fewer idle cpus available for scheduling. "
                    "So do not need enable docker coordination burst.";
    }
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
}

void DockerCoordinationBurstAnalysis::Reset(const std::string &topicType)
{
    topicStatus[topicType].hostCpuUtil = 0;
    topicStatus[topicType].hostCpuUtilSum = 0;
    topicStatus[topicType].sampleCount = 0;
    topicStatus[topicType].hostCpuUsageThreshold = HOST_CPU_USAGE_THRESHOLD_DEFAULT;
    topicStatus[topicType].dockerCpuUsagethreshold = DOCKER_CPU_USAGE_THRESHOLD_DEFAULT;
    topicStatus[topicType].dockerList.clear();
}

void DockerCoordinationBurstAnalysis::Run()
{
    auto now = std::chrono::system_clock::now();
    for (auto &item : topicStatus) {
        auto &status = item.second;
        if (status.isOpen) {
            int curTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - status.beginTime).count();
            if (!status.isPublish && curTimeMs / MS_PER_SEC >= status.time) {
                auto topicType = item.first;
                const auto topic = Topic::GetTopicFromType(topicType);
                Analysis(topicType);
                PublishData(topic);
                status.isPublish = true;
            }
        }
    }
}

void DockerInfo::UpdateCpuUtil(const uint64_t cpuUsage, const uint64_t samplingTimeStamp)
{
    if (startTimestamp == 0) {
        startTimestamp = samplingTimeStamp;
        startCpuUsage = cpuUsage;
    } else {
        endTimeStamp = samplingTimeStamp;
        endCpuUsage = cpuUsage;
    }
}

double DockerInfo::GetCpuUtil() const  // 0-100
{
    if (startTimestamp == endTimeStamp || startTimestamp == 0 || endTimeStamp == 0) {
        return 0.0;
    }
    int cpuLimit = hostCpuNum;
    if (cfsQuotaUs > 0 && cfsPeriodUs > 0) {
        cpuLimit = cfsQuotaUs / cfsPeriodUs;
    }

    uint64_t cpuUsageDelta = endCpuUsage - startCpuUsage;
    uint64_t interval = endTimeStamp - startTimestamp;

    double cpuUtil = (cpuUsageDelta / 1e9) / (cpuLimit * (static_cast<double>(interval) / 1e9)) * PERCENTAGE_FACTOR;
    return cpuUtil;
}

}  // namespace oeaware
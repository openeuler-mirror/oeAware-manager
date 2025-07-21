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
#include "numa_analysis.h"
#include <string>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <log4cplus/log4cplus.h>
#include <log4cplus/loglevel.h>
#include "oeaware/utils.h"
#include "analysis_utils.h"
#include "oeaware/interface.h"
#include "oeaware/data/thread_info.h"
#include "data_register.h"
#include "oeaware/data/pmu_counting_data.h"
#include "oeaware/data/pmu_uncore_data.h"
#include "oeaware/data/analysis_data.h"

namespace oeaware {

const int MS_PER_SEC = 1000;
const int DEFAULT_TIME_WINDOW_MS = 100;
const int MAX_THREAD_THRESHOLD = 10000;
const double NUMA_OPS_THRESHOLD_PER_SEC = 2000000; // Including oeaware analysis background noise
const double NUMA_REMOTE_ACCESS_RATIO_THRESHOLD = 5.0;
const double PERCENT_CONVERSION = 100.0;

NumaAnalysis::NumaAnalysis()
{
    name = OE_NUMA_ANALYSIS;
    period = ANALYSIS_TIME_PERIOD;
    priority = 1;
    type = SCENARIO;
    version = "1.0.0";
    for (auto &topic : topicStrs) {
        supportTopics.emplace_back(Topic{name, topic, ""});
    }
}

Result NumaAnalysis::Enable(const std::string &param)
{
    (void)param;
    return Result(OK);
}

void NumaAnalysis::Disable()
{
    AnalysisResultItemFree(&analysisResultItem);
}

Result NumaAnalysis::OpenTopic(const oeaware::Topic &topic)
{
    if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
        return Result(FAILED, "topic " + topic.topicName + " not support!");
    }

    auto topicType = topic.GetType();
    auto params_map = GetKeyValueFromString(topic.params);
    if (params_map.count("t")) {
        time = atoi(params_map["t"].data());
    }
    if (params_map.count("threshold")) {
        if (atoi(params_map["threshold"].data()) >= 1 &&
            atoi(params_map["threshold"].data()) <= MAX_THREAD_THRESHOLD) {
            threadThreshold = atoi(params_map["threshold"].data());
        } else {
            ERROR(logger, "Invalid threshold value: " << params_map["threshold"].data() << "!");
            return Result(FAILED, "topic" + topicType + " open failed! Invalid threshold value!");
        }
    }

    topicStatus[topicType].open = true;
    beginTime = std::chrono::high_resolution_clock::now();

    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "sched:sched_process_fork", ""});
    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "sched:sched_process_exit", ""});
    subTopics.emplace_back(oeaware::Topic{OE_PMU_UNCORE_COLLECTOR, "uncore", ""});

    for (auto &subTopic : subTopics) {
        Subscribe(subTopic);
    }

    oeaware::ReadSchedFeatures(schedFeaturePath, schedFeatues);
    return Result(OK);
}

void NumaAnalysis::CloseTopic(const oeaware::Topic &topic)
{
    auto topicType = topic.GetType();
    topicStatus[topicType].open = false;
    topicStatus[topicType].isPublish = false;
    for (auto &subTopic : subTopics) {
        Unsubscribe(subTopic);
    }
    subTopics.clear();
}

void NumaAnalysis::UpdateData(const DataList &dataList)
{
    std::string instanceName = dataList.topic.instanceName;
    std::string topicName = dataList.topic.topicName;

    if (instanceName == OE_PMU_COUNTING_COLLECTOR) {
        auto countingData = static_cast<PmuCountingData*>(dataList.data[0]);
        if (topicName == "sched:sched_process_fork" && countingData->len > 0) {
            for (int i = 0; i < countingData->len; i++) {
                threadCreatedCount += countingData->pmuData[i].count;
            }
            timeWindowMs = countingData->interval;
        } else if (topicName == "sched:sched_process_exit" && countingData->len > 0) {
            for (int i = 0; i < countingData->len; i++) {
                threadDestroyedCount += countingData->pmuData[i].count;
            }
            timeWindowMs = countingData->interval;
        }
    } else if (topicName == "uncore") {
        auto uncoreData = static_cast<PmuUncoreData*>(dataList.data[0]);
        totalOpsNum = 0;
        totalRxOuter = 0;
        totalRxSccl = 0;

        int hhaNum = uncoreData->len / 3; // 3 events：rx_outer, rx_sccl, rx_ops_num
        if (hhaNum <= 0) {
            return;
        }

        for (int i = 0; i < hhaNum; i++) {
            totalRxOuter += uncoreData->pmuData[i].count;
            totalRxSccl += uncoreData->pmuData[i + hhaNum].count;
            totalOpsNum += uncoreData->pmuData[i + 2 * hhaNum].count;
        }

        isNumaBottleneck = CheckNumaBottleneck();
    }
}

bool NumaAnalysis::CheckNumaBottleneck()
{
    double opsNumPerSecond = 0.0;
    if (timeWindowMs > 0) {
        opsNumPerSecond = static_cast<double>(totalOpsNum) * MS_PER_SEC / timeWindowMs;
    }

    if (opsNumPerSecond <= NUMA_OPS_THRESHOLD_PER_SEC) {
        return false;
    }

    if (totalOpsNum > 0) {
        remoteAccessRatio = static_cast<double>(totalRxOuter + totalRxSccl) / totalOpsNum * PERCENT_CONVERSION;
        return remoteAccessRatio > NUMA_REMOTE_ACCESS_RATIO_THRESHOLD;
    }

    return false;
}

void NumaAnalysis::Analysis()
{
    std::vector<int> type;
    std::vector<std::vector<std::string>> metrics;

    CalculateAndAddMetrics(type, metrics);
    GenerateConclusion(type, metrics);
}

void NumaAnalysis::CalculateAndAddMetrics(std::vector<int>& type,
                                          std::vector<std::vector<std::string>>& metrics)
{
    double threadCreatedPerSecond = CalculateThreadCreationRate();
    double opsNumPerSecond = CalculateOpsRate();

    // NUMA operations metric
    type.emplace_back(DATA_TYPE_CPU);
    metrics.emplace_back(std::vector<std::string>{
        "uncore_ops_num_per_second", std::to_string(opsNumPerSecond),
        (opsNumPerSecond > NUMA_OPS_THRESHOLD_PER_SEC ? "high mem access" : "low mem access")});

    // Remote access ratio metric
    if (isNumaBottleneck) {
        type.emplace_back(DATA_TYPE_CPU);
        metrics.emplace_back(std::vector<std::string>{
            "remote_access_ratio", std::to_string(remoteAccessRatio) + "%",
            (remoteAccessRatio > NUMA_REMOTE_ACCESS_RATIO_THRESHOLD ? "high remote mem access"
                                                                    : "low remote mem access")});
    }

    // Thread creation metric
    type.emplace_back(DATA_TYPE_CPU);
    metrics.emplace_back(std::vector<std::string>{
        "thread_created_per_second", std::to_string(threadCreatedPerSecond),
        (threadCreatedPerSecond > threadThreshold ? "high thread creation"
                                                  : "low thread creation")});
}

void NumaAnalysis::GenerateConclusion(std::vector<int>& type,
                                      std::vector<std::vector<std::string>>& metrics)
{
    std::string conclusion;
    std::vector<std::string> suggestionItem;
    double threadCreatedPerSecond = CalculateThreadCreationRate();

    if (isNumaBottleneck) {
        if (threadCreatedPerSecond > threadThreshold) {
            if (IsSupportNumaSchedParal()) {
                conclusion =
                    "NUMA access bottleneck with high thread creation detected. Enable NUMA native "
                    "scheduling recommended.";
                suggestionItem.emplace_back("Enable NUMA native scheduling");
                suggestionItem.emplace_back("oeawarectl -e numa_sched");
                suggestionItem.emplace_back(
                    "Reduces cross-NUMA access for applications with frequent thread creation");
            } else {
                conclusion =
                    "NUMA access bottleneck with high thread creation detected, but numa parallel "
                    "schedule not support.";
            }
        } else {
            conclusion = "NUMA access bottleneck detected. Enable NUMA Fast scheduling recommended.";
            suggestionItem.emplace_back("Use numafast");
            suggestionItem.emplace_back(
                "step 1: if  `oeawarectl -q | grep tune_numa_mem_access` not exist, install "
                "numafast\ninstall : `oeawarectl -i numafast`\nload :    `oeawarectl -l "
                "libtune_numa.so`\nstep 2: enable instance `oeaware -e tune_numa_mem_access`\n");
            suggestionItem.emplace_back(
                "Optimizes memory access locality with low scheduling overhead");
        }
    } else {
        conclusion = "NUMA is not a performance bottleneck. No NUMA-related optimization needed.";
    }

    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
}

double NumaAnalysis::CalculateThreadCreationRate()
{
    if (timeWindowMs <= 0) {
        return 0.0;
    }
    return static_cast<double>(threadCreatedCount) * MS_PER_SEC / timeWindowMs;
}

double NumaAnalysis::CalculateOpsRate()
{
    if (timeWindowMs <= 0) {
        return 0.0;
    }
    return static_cast<double>(totalOpsNum) * MS_PER_SEC / timeWindowMs;
}

void* NumaAnalysis::GetResult()
{
    return &analysisResultItem;
}

void NumaAnalysis::PublishData(const Topic &topic)
{
    DataList dataList;
    SetDataListTopic(&dataList, topic.instanceName, topic.topicName, topic.params);
    dataList.len = 1;
    dataList.data = new void*[dataList.len];
    dataList.data[0] = GetResult();
    Publish(dataList, false);
}

void NumaAnalysis::Reset()
{
    threadCreatedCount = 0;
    threadDestroyedCount = 0;
    timeWindowMs = DEFAULT_TIME_WINDOW_MS;
    totalOpsNum = 0;
    totalRxOuter = 0;
    totalRxSccl = 0;
    remoteAccessRatio = 0.0;
    isNumaBottleneck = false;
}

void NumaAnalysis::Run()
{
    auto now = std::chrono::system_clock::now();

    for (auto& item : topicStatus) {
        auto& status = item.second;
        if (status.open) {
            int curTimeMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - beginTime).count();
            if (!status.isPublish && curTimeMs / MS_PER_SEC >= time) {
                auto topicType = item.first;
                const auto& topic = Topic::GetTopicFromType(topicType);
                Analysis();
                PublishData(topic);
                Reset();
                status.isPublish = true;
            }
        }
    }
}

bool NumaAnalysis::IsSupportNumaSchedParal()
{
    for (const auto &feature : schedFeatues) {
        if (feature == "PARAL" || feature == "NO_PARAL") {
            return true;
        }
    }
    return false;
}

}  // namespace oeaware
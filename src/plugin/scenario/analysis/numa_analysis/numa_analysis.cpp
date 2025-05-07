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
#include "oeaware/data/analysis_data.h"
#include "oeaware/logger.h"
#include "oeaware/utils.h"
#include "analysis_utils.h"
#include "oeaware/interface.h"
#include "oeaware/data/thread_info.h"
#include "data_register.h"
#include "oeaware/data/pmu_counting_data.h"
#include "oeaware/data/pmu_uncore_data.h"
#include <yaml-cpp/yaml.h>

namespace oeaware {

const int MS_PER_SEC = 1000;
const double NUMA_OPS_THRESHOLD_PER_SEC = 2000000; // 包含oeaware分析自身的底噪
const double NUMA_REMOTE_ACCESS_RATIO_THRESHOLD = 5.0;

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

    LoadConfig();
}

Result NumaAnalysis::Enable(const std::string &param)
{
    (void)param;
    return Result(OK);
}

void NumaAnalysis::Disable()
{
}

Result NumaAnalysis::OpenTopic(const oeaware::Topic &topic)
{
    if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
        return Result(FAILED, "topic " + topic.topicName + " not support!");
    }
    auto topicType = topic.GetType();
    topicStatus[topicType].open = true;
    beginTime = std::chrono::high_resolution_clock::now();

    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "sched:sched_process_fork", ""});
    subTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "sched:sched_process_exit", ""});
    subTopics.emplace_back(oeaware::Topic{OE_PMU_UNCORE_COLLECTOR, "uncore", ""});

    for (auto &subTopic : subTopics) {
        Subscribe(subTopic);
    }

    auto params_map = GetKeyValueFromString(topic.params);
    if (params_map.count("t")) {
        time = atoi(params_map["t"].data());
    }
    if (params_map.count("threshold")) {
        threadThreshold = atoi(params_map["threshold"].data());
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
    // 将totalOpsNum归一化为每秒的操作数
    double opsNumPerSecond = 0.0;
    if (timeWindowMs > 0) {
        opsNumPerSecond = static_cast<double>(totalOpsNum) * 1000.0 / timeWindowMs;
    }
    
    // 使用归一化后的每秒操作数进行判断
    if (opsNumPerSecond <= NUMA_OPS_THRESHOLD_PER_SEC) {
        return false;
    }

    if (totalOpsNum > 0) {
        remoteAccessRatio = static_cast<double>(totalRxOuter + totalRxSccl) / totalOpsNum * 100.0;
        return remoteAccessRatio > NUMA_REMOTE_ACCESS_RATIO_THRESHOLD;
    }

    return false;
}

void NumaAnalysis::Analysis()
{
    std::vector<int> type;
    std::vector<std::vector<std::string>> metrics;

    // 计算每秒创建的线程数
    double threadCreatedPerSecond = 0.0;
    if (timeWindowMs > 0) {
        threadCreatedPerSecond = static_cast<double>(threadCreatedCount) * 1000.0 / timeWindowMs;
    }

    // 计算每秒的操作数
    double opsNumPerSecond = 0.0;
    if (timeWindowMs > 0) {
        opsNumPerSecond = static_cast<double>(totalOpsNum) * 1000.0 / timeWindowMs;
    }

    // numa is bottleneck
    type.emplace_back(DATA_TYPE_CPU);
    metrics.emplace_back(std::vector<std::string>{
        "uncore_ops_num_per_second", std::to_string(opsNumPerSecond),
            (opsNumPerSecond > NUMA_OPS_THRESHOLD_PER_SEC ? "high mem access" : "low mem access")});
    if (isNumaBottleneck) {
        type.emplace_back(DATA_TYPE_CPU);
        metrics.emplace_back(std::vector<std::string>{
            "remote_access_ratio", std::to_string(remoteAccessRatio) + "%",
                (remoteAccessRatio > NUMA_REMOTE_ACCESS_RATIO_THRESHOLD ? "high remote mem access" : "low remote mem access")});
    }

    // thread creation
    type.emplace_back(DATA_TYPE_CPU);
    metrics.emplace_back(std::vector<std::string>{
        "thread_created_per_second", std::to_string(threadCreatedPerSecond),
            (threadCreatedPerSecond > threadThreshold ? "high thread creation"
                : "low thread creation")});

    std::string conclusion;
    std::vector<std::string> suggestionItem;

    // 使用归一化后的每秒线程创建数进行判断
    if (isNumaBottleneck && threadCreatedPerSecond > threadThreshold) {
        conclusion = "NUMA access bottleneck with high thread creation detected.";
        if (IsSupportNumaSchedParal()) {
            conclusion += " SCHED_PARAL recommended.";
            suggestionItem.emplace_back("Enable NUMA native scheduling");
            suggestionItem.emplace_back("echo SCHED_PARAL > " + schedFeaturePath);
            suggestionItem.emplace_back(
                "Reduces cross-NUMA access for applications with frequent thread creation");
        } else {
            conclusion += "SCHED_PARAL not supported.";
        }
    }

    if (isNumaBottleneck && (threadCreatedPerSecond <= threadThreshold || !IsSupportNumaSchedParal())) {
        conclusion = "NUMA access bottleneck detected. Enable NUMA Fast scheduling recommended.";
        suggestionItem.emplace_back("Use numafast");
        suggestionItem.emplace_back(
            "step 1: if  `oeawarectl -q | grep tune_numa_mem_access` not exist, install "
            "numafast\ninstall : `oeawarectl -i numafast`\nload :    `oeawarectl -l "
            "libtune_numa.so`\nstep 2: enable instance `oeaware -e tune_numa_mem_access`\n");
        suggestionItem.emplace_back(
            "Optimizes memory access locality with low scheduling overhead");
    } else {
        conclusion = "NUMA is not a performance bottleneck. No NUMA-related optimization needed.";
    }

    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
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
    Publish(dataList);
}

void NumaAnalysis::Reset()
{
    threadCreatedCount = 0;
    threadDestroyedCount = 0;
    timeWindowMs = 100;
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

void NumaAnalysis::LoadConfig()
{
    try {
        YAML::Node config = YAML::LoadFile("/etc/oeAware/analysis_config.yaml");

        if (config["numa_native_sched"] && config["numa_native_sched"]["thread_threshold"]) {
            int configThreadThreshold = config["numa_native_sched"]["thread_threshold"].as<int>();
            if (configThreadThreshold >= 1 && configThreadThreshold <= 10000) {
                threadThreshold = configThreadThreshold;
            }
        }
    } catch (const std::exception& e) {
        // 配置文件加载失败，使用默认值或已有值
    }
}

bool NumaAnalysis::IsSupportNumaSchedParal()
{
    for (const auto &feature : schedFeatues) {
        if (feature == "SCHED_PARAL" || feature == "NO_SCHED_PARAL") {
            return true;
        }
    }
    return false;
}

}  // namespace oeaware
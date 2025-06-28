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
#include "dynamic_smt_analysis.h"
#include "oeaware/utils.h"
#include "env_data.h"
#include "analysis_utils.h"

namespace oeaware {
DynamicSmtAnalysis::DynamicSmtAnalysis()
{
    name = OE_DYNAMIC_SMT_ANALYSIS;
    period = ANALYSIS_TIME_PERIOD;
    priority = 1;
    type = SCENARIO;
    version = "1.0.0";
    for (auto &topic : topicStrs) {
        supportTopics.emplace_back(Topic{name, topic, ""});
    }
    subscribeTopics.emplace_back(oeaware::Topic{OE_ENV_INFO, "cpu_util", ""});
}

Result DynamicSmtAnalysis::Enable(const std::string &param)
{
    (void)param;
    return Result(OK);
}

void DynamicSmtAnalysis::Disable()
{
    AnalysisResultItemFree(&analysisResultItem);
    topicStatus.clear();
}

Result DynamicSmtAnalysis::OpenTopic(const oeaware::Topic &topic)
{
    if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
        return Result(FAILED, "topic " + topic.topicName + " not support!");
    }
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }
    auto topicType = topic.GetType();
    topicStatus[topicType].isOpen = true;
    topicStatus[topicType].beginTime = std::chrono::high_resolution_clock::now();
    auto paramsMap = GetKeyValueFromString(topic.params);
    if (paramsMap.count("t")) {
        topicStatus[topicType].time = atoi(paramsMap["t"].data());
    }
    if (paramsMap.count("threshold")) {
        topicStatus[topicType].threshold = atof(paramsMap["threshold"].data());
    }
    return Result(OK);
}

void DynamicSmtAnalysis::CloseTopic(const oeaware::Topic &topic)
{
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    auto topicType = topic.GetType();
    topicStatus[topicType].isOpen = false;
    topicStatus[topicType].isPublish = false;
    topicStatus[topicType].cpuSum = 0;
    topicStatus[topicType].cpuIdle = 0;
    topicStatus[topicType].threshold = DYNAMIC_SMT_THRESHOLD;
}

void DynamicSmtAnalysis::UpdateData(const DataList &dataList)
{
    std::string instanceName = dataList.topic.instanceName;
    std::string topicName = dataList.topic.topicName;
    for (auto &p : topicStatus) {
        auto topicType = p.first;
        const auto &topic = Topic::GetTopicFromType(topicType);
        if (p.second.isOpen) {
            auto cpuUtilData = static_cast<EnvCpuUtilParam*>(dataList.data[0]);
            if (cpuUtilData->dataReady != ENV_DATA_READY) {
                continue;
            }
            p.second.cpuIdle = cpuUtilData->times[cpuUtilData->cpuNumConfig][CPU_IDLE];
            p.second.cpuSum = cpuUtilData->times[cpuUtilData->cpuNumConfig][CPU_TIME_SUM];
        }
    }
}

void DynamicSmtAnalysis::PublishData(const Topic &topic)
{
    DataList dataList;
    SetDataListTopic(&dataList, topic.instanceName, topic.topicName, topic.params);
    dataList.len = 1;
    dataList.data = new void *[dataList.len];
    dataList.data[0] = &analysisResultItem;
    Publish(dataList, false);
}

void DynamicSmtAnalysis::Run()
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

void DynamicSmtAnalysis::Analysis(const std::string &topicType)
{
    std::vector<int> type;
    std::vector<std::vector<std::string>> metrics;
    type.emplace_back(DATA_TYPE_CPU);
    auto cpuUsage = (topicStatus[topicType].cpuSum - topicStatus[topicType].cpuIdle) / topicStatus[topicType].cpuSum;
    metrics.emplace_back(std::vector<std::string>{"cpu_usage", std::to_string(cpuUsage *PERCENTAGE_FACTOR) + "%",
        (cpuUsage *PERCENTAGE_FACTOR >= topicStatus[topicType].threshold ? "high for dynamic smt tune" :
            "low for dynamic smt tune")});
    std::string conclusion;
    std::vector<std::string> suggestionItem;
    if (cpuUsage * PERCENTAGE_FACTOR < topicStatus[topicType].threshold) {
        if (IsTuneSupport()) {
            conclusion = "For dynamic_smt_tune, the system loads is low. Please enable dynamic_smt_tune";
            suggestionItem.emplace_back("use dynamic smt");
            suggestionItem.emplace_back("oeawarectl -e dynamic_smt_tune");
            suggestionItem.emplace_back("activate idle physical cores during low system loads to improve performance");
        } else {
            conclusion = "Dynamic_smt_tune is not supported.";
        }
    } else {
        conclusion = "For dynamic_smt_tune, donot need to enable dynamic smt.";
    }
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
}
bool DynamicSmtAnalysis::IsTuneSupport()
{
    if (!oeaware::IsSmtEnable()) {
        return false;
    }

    std::vector<std::string> features;
    std::string path;
    oeaware::ReadSchedFeatures(path, features);
    for (const auto &feature : features) {
        if (feature == "KEEP_ON_CORE" || feature == "NO_KEEP_ON_CORE") {
            return true;
        }
    }
    return false;
}
}

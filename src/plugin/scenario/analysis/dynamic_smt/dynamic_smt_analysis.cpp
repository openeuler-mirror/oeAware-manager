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
#include "oeaware/utils.h"
#include "analysis_utils.h"
#include "env_data.h"
#include "dynamic_smt_analysis.h"

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
}

Result DynamicSmtAnalysis::Enable(const std::string &param)
{
    auto paramsMap = GetKeyValueFromString(param);
    if (paramsMap.count("t")) {
        time = atoi(paramsMap["t"].data());
    }
    if (paramsMap.count("threshold")) {
        threshold = atof(paramsMap["threshold"].data());
    }
    return Result(OK);
}

void DynamicSmtAnalysis::Disable()
{
}

Result DynamicSmtAnalysis::OpenTopic(const oeaware::Topic &topic)
{
    if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
        return Result(FAILED, "topic " + topic.topicName + " not support!");
    }
    auto topicType = topic.GetType();
    topicStatus[topicType] = true;
    subscribeTopics.emplace_back(oeaware::Topic{OE_ENV_INFO, "cpu_util", ""});
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }
    return Result(OK);
}

void DynamicSmtAnalysis::CloseTopic(const oeaware::Topic &topic)
{
    auto topicType = topic.GetType();
    topicStatus[topicType] = false;
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    subscribeTopics.clear();
}

void DynamicSmtAnalysis::UpdateData(const DataList &dataList)
{
    std::string instanceName = dataList.topic.instanceName;
    std::string topicName = dataList.topic.topicName;
    for (auto &p : topicStatus) {
        auto topicType = p.first;
        const auto &topic = Topic::GetTopicFromType(topicType);
        if (p.second) {
            auto cpuUtilData = static_cast<EnvCpuUtilParam*>(dataList.data[0]);
            auto cpuIdle = cpuUtilData->times[cpuUtilData->cpuNumConfig][CPU_IDLE];
            auto cpuSum = cpuUtilData->times[cpuUtilData->cpuNumConfig][CPU_TIME_SUM];
            cpuUsage = static_cast<double>(cpuSum - cpuIdle) / cpuSum;
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
    Publish(dataList);
}

void DynamicSmtAnalysis::Run()
{
    curTime++;
    if (curTime == time) {
        for (auto &item : topicStatus) {
            auto &status = item.second;
            auto topicType = item.first;
            const auto &topic = Topic::GetTopicFromType(topicType);
            if (status) {
                Analysis();
                PublishData(topic);
            }
        }
        curTime = 0;
    }
}

void DynamicSmtAnalysis::Analysis()
{
    std::vector<std::vector<std::string>> metrics;
    metrics.emplace_back(std::vector<std::string>{"cpu_usage", std::to_string(cpuUsage * PERCENTAGE_FACTOR) + "%",
        (cpuUsage * PERCENTAGE_FACTOR > threshold ? "high for dynamic smt tune" : "low for dynamic smt tune")});
    std::string conclusion;
    std::vector<std::string> suggestionItem;
    if (cpuUsage * PERCENTAGE_FACTOR < threshold) {
        conclusion = "For dynamic_smt_tune, the system loads is low. Please enable dynamic_smt_tune";
        suggestionItem.emplace_back("use dynamic smt");
        suggestionItem.emplace_back("oeawarectl -e dynamic_smt_tune");
        suggestionItem.emplace_back("activate idle physical cores during low system loads to improveperformance");
    } else {
        conclusion = "For dynamic_smt_tune, donot need to enable dynamic smt.";
    }
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, &analysisResultItem);
}
}
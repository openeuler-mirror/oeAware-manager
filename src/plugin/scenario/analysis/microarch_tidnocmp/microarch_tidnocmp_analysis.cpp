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
#include "microarch_tidnocmp_analysis.h"
#include <string>
#include <iostream>
#include <sstream>
#include <yaml-cpp/yaml.h>
#include <securec.h>
#include "oeaware/data/analysis_data.h"
#include "oeaware/utils.h"
#include "oeaware/interface.h"
#include "oeaware/data_list.h"
#include "analysis_utils.h"
#include "data_register.h"
#include "thread_info.h"

const int MS_PER_SEC = 1000;

namespace oeaware {
MicroarchTidNoCmpAnalysis::MicroarchTidNoCmpAnalysis()
{
    name = OE_MICRO_ARCH_TIDNOCMP_ANALYSIS;
    period = ANALYSIS_TIME_PERIOD;
    priority = 1;
    type = SCENARIO;
    version = "1.0.0";
    for (auto &topic : topicStrs) {
        supportTopics.emplace_back(Topic{name, topic, ""});
    }
    cpuPartId = GetCpuPartId();
}

bool MicroarchTidNoCmpAnalysis::ParseConfig(const std::string &topicType, const std::string &param)
{
    auto paramsMap = GetKeyValueFromString(param);
    if (paramsMap.count("t")) {
        topicStatus[topicType].time = atoi(paramsMap["t"].data());
    }
    if (!paramsMap.count("micro_arch_tidnocmp_config_stream")) {
        return false;
    }

    std::istringstream iss(paramsMap["micro_arch_tidnocmp_config_stream"].data());
    try {
        YAML::Node parsedConfig = YAML::Load(iss);
        if (!parsedConfig["service_list"] || !parsedConfig["service_list"].IsSequence()) {
            WARN(logger, "Missing or invalid 'service_list' node in config");
            return false;
        }
        if (!parsedConfig["cpu_part"] || !parsedConfig["cpu_part"].IsSequence()) {
            WARN(logger, "Missing or invalid 'cpu_part' node in config");
            return false;
        }

        for (auto &&service : parsedConfig["service_list"]) {
            topicStatus[topicType].supportServiceList.insert(service.as<std::string>());
        }
        for (auto &&cpu : parsedConfig["cpu_part"]) {
            topicStatus[topicType].supportCpuPartId.insert(cpu.as<std::string>());
        }
    } catch (const YAML::Exception &e) {
        ERROR(logger, "MicroarchTidNoCmpAnalysis ParseConfig yaml config format is invalid.");
        return false;
    }
    return true;
}

Result MicroarchTidNoCmpAnalysis::Enable(const std::string &param)
{
    (void)param;
    return Result(OK);
}

void MicroarchTidNoCmpAnalysis::Disable()
{
    AnalysisResultItemFree(&analysisResultItem);
    topicStatus.clear();
}

Result MicroarchTidNoCmpAnalysis::OpenTopic(const oeaware::Topic &topic)
{
    if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
        return Result(FAILED, "topic " + topic.topicName + " not support!");
    }
    auto topicType = topic.GetType();
    topicStatus[topicType].isOpen = true;
    topicStatus[topicType].beginTime = std::chrono::high_resolution_clock::now();
    cpuPartId = GetCpuPartId();
    subscribeTopics.emplace_back(oeaware::Topic{OE_THREAD_COLLECTOR, OE_THREAD_COLLECTOR, ""});
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }

    if (!ParseConfig(topicType, topic.params)) {
        return Result(FAILED, "MicroarchTidNoCmp analysis config is Invalid.");
    }

    return Result(OK);
}

void MicroarchTidNoCmpAnalysis::CloseTopic(const oeaware::Topic &topic)
{
    auto topicType = topic.GetType();
    topicStatus[topicType].isOpen = false;
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    Reset(topicType);
}

void MicroarchTidNoCmpAnalysis::UpdateThreadData(const std::string &topicType, const DataList &dataList)
{
    for (uint64_t i = 0; i < dataList.len; ++i) {
        ThreadInfo *threadData = (ThreadInfo *)(dataList.data[i]);
        std::string threadName = threadData->name;
        for (auto supportService : topicStatus[topicType].supportServiceList) {
            if (threadName == supportService) {
                topicStatus[topicType].catchedThreadList.insert(threadName);
                break;
            }
        }
    }
}

void MicroarchTidNoCmpAnalysis::UpdateData(const DataList &dataList)
{
    std::string topicName = dataList.topic.topicName;
    if (topicName != OE_THREAD_COLLECTOR) {
        return;
    }

    for (auto &p : topicStatus) {
        if (p.second.isOpen) {
            UpdateThreadData(p.first, dataList);
        }
    }
}

void MicroarchTidNoCmpAnalysis::PublishData(const Topic &topic)
{
    DataList dataList;
    SetDataListTopic(&dataList, topic.instanceName, topic.topicName, topic.params);
    dataList.len = 1;
    dataList.data = new void *[dataList.len];
    dataList.data[0] = &analysisResultItem;
    Publish(dataList, false);
}

std::string MicroarchTidNoCmpAnalysis::JoinThreadList(const std::set<std::string> &list) const
{
    std::ostringstream oss;
    for (auto it = list.begin(); it != list.end();) {
        oss << *it;
        ++it;
        if (it != list.end()) {
            oss << ",";
        }
    }
    return oss.str();
}

void MicroarchTidNoCmpAnalysis::Analysis(const std::string &topicType)
{
    std::vector<std::vector<std::string>> metrics;
    std::vector<int> type;
    std::string conclusion;
    std::vector<std::string> suggestionItem;
    auto analysisTopicStatus = topicStatus[topicType];
    std::string suggestionThreads = JoinThreadList(analysisTopicStatus.catchedThreadList);
    std::string supportService = JoinThreadList(analysisTopicStatus.supportServiceList);

    if (analysisTopicStatus.supportCpuPartId.find(cpuPartId) !=
        analysisTopicStatus.supportCpuPartId.end()) {
        if (analysisTopicStatus.catchedThreadList.empty()) {
            conclusion = "Can not find service in [" + supportService +
                         "] running in the current environment. So do not need to enable TidNoCmp.";
        } else {
            conclusion = "The TidNoCmp applicable service [" + suggestionThreads +
                         "] is running in the current environment. So enable TidNoCmp.";
            suggestionItem.emplace_back("use micro-arch TidNoCmp");
            suggestionItem.emplace_back("Please set bios config [Advanced -> Power And Performance "
                                        "Configuration -> CPU PM Control -> TidNoCMP] to [Enabled].");
            suggestionItem.emplace_back(
                "Disable the isolation of thread branch prediction records to "
                "improve the performance of the supported service.");
        }
    } else {
        conclusion = "The cpu model of this environment is " + cpuPartId +
                     ", not in support CPU model config list .";
    }
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
}

void MicroarchTidNoCmpAnalysis::Reset(const std::string &topicType)
{
    topicStatus[topicType].catchedThreadList.clear();
    topicStatus[topicType].supportCpuPartId.clear();
    topicStatus[topicType].supportServiceList.clear();
}

void MicroarchTidNoCmpAnalysis::Run()
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

}  // namespace oeaware
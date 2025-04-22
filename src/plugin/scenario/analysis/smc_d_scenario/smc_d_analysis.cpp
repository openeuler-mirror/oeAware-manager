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
#include <string>
#include <iostream>
#include "oeaware/data/analysis_data.h"
#include "oeaware/utils.h"
#include "analysis_utils.h"
#include "smc_d_analysis.h"

constexpr int BUFFER_SIZE = 128;

namespace oeaware {
SmcDAnalysis::SmcDAnalysis()
{
    name = OE_SMCD_ANALYSIS;
    period = ANALYSIS_TIME_PERIOD;
    priority = 1;
    type = SCENARIO;
    version = "1.0.0";
    description = "An instance of SMC-D feature applicability for context-aware scene perception.";
    for (auto &topic : topicStrs) {
        supportTopics.emplace_back(Topic{name, topic, ""});
    }
}

Result SmcDAnalysis::OpenTopic(const Topic &topic)
{
    if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
        return Result(FAILED, "topic " + topic.topicName + " not support!");
    }
    saveTopic = topic;
    return Result(OK);
}

void SmcDAnalysis::CloseTopic(const Topic &topic)
{
    (void)topic;
}

Result SmcDAnalysis::Enable(const std::string &param)
{
    try {
        auto params_map = GetKeyValueFromString(param);
        if (params_map.count("t")) {
            analysisTime = atoi(params_map["t"].data());
        }
        return Result(OK);
    } catch (...) {
        return Result(FAILED);
    }
}

void SmcDAnalysis::Disable()
{
    prevEstablishedCount = 0;
    prevCloseWaitCount = 0;
    totalEstablishedDelta = 0;
    totalCloseWaitDelta = 0;
    curTime = 0;
}

void SmcDAnalysis::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

std::string SmcDAnalysis::ExecuteCommand(const std::string &cmd)
{
    std::array<char, BUFFER_SIZE> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

int SmcDAnalysis::GetConnectionCount(const std::string &state)
{
    std::string cmd = "grep -E '^ *[0-9]+:' /proc/net/tcp | awk '{if ($4 == \""
                    + state + "\") count++} END {print count+0}'";
    std::string output = ExecuteCommand(cmd);
    try {
        return std::stoi(output);
    } catch (...) {
        return 0;
    }
}

void SmcDAnalysis::DetectTcpConnectionMode()
{
    int currentEstablishedCount = GetConnectionCount("01"); // ESTABLISHED
    int currentCloseWaitCount = GetConnectionCount("08");   // CLOSE_WAIT
    int establishedDelta = std::abs(currentEstablishedCount - prevEstablishedCount);
    int closeWaitDelta = std::abs(currentCloseWaitCount - prevCloseWaitCount);
    prevEstablishedCount = currentEstablishedCount;
    prevCloseWaitCount = currentCloseWaitCount;
    if (curTime == 0) {
        return;
    }
    totalEstablishedDelta += establishedDelta;
    totalCloseWaitDelta += closeWaitDelta;
}

void *SmcDAnalysis::GetResult()
{
    const char metricName[] = "SmcDTcpChangeRate";
    const char suggestionSmcEnabled[] = "use smc-d";
    const char suggestionSmcDisabled[] = "no use smc-d";
    const char conclusionSilent[] = "Current scenario detects fewer or no connections.";
    const char conclusionLongConn[] = "Current scenario detects long-lived connections.";
    const char conclusionShortConn[] = "Current scenario detects short-lived connections.";
    std::vector<int> type;
    type.emplace_back(DATA_TYPE_NETWORK);
    AnalysisResultItem *result = new AnalysisResultItem;
    double totalEstablishedRate = 0;
    if (prevEstablishedCount != 0) {
        totalEstablishedRate = totalEstablishedDelta / static_cast<double>(prevEstablishedCount);
    }
    bool isSilence = prevEstablishedCount < 5;
    std::vector<std::vector<std::string>> metrics;
    std::string conclusion;
    std::vector<std::string> suggestionItem;
    if (isSilence) {
        metrics.emplace_back(std::vector<std::string>{
            metricName, "low", "stable"
        });
        conclusion = conclusionSilent;
        suggestionItem.emplace_back(suggestionSmcDisabled);
        suggestionItem.emplace_back("");
        suggestionItem.emplace_back("");
    } else {
        bool isSmcDUsable = totalEstablishedRate < 0.1 && totalCloseWaitDelta < 10;
        metrics.emplace_back(std::vector<std::string>{
            metricName,
            std::to_string(totalEstablishedRate),
            isSmcDUsable ? "low" : "high",
        });
        conclusion = isSmcDUsable ? conclusionLongConn : conclusionShortConn;
        suggestionItem.emplace_back(isSmcDUsable ? suggestionSmcEnabled : suggestionSmcDisabled);
        suggestionItem.emplace_back(isSmcDUsable ? "oeawarectl -e smc_tune" : "");
        suggestionItem.emplace_back(isSmcDUsable ? "Reduce latency and increase throughput."
                                                 : "Enabling it will lead to degradation.");
    }
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, result);
    return result;
}

void SmcDAnalysis::PublishData()
{
    void *rawResult = GetResult();
    AnalysisResultItem *result = static_cast<AnalysisResultItem *>(rawResult);
    DataList dataList;
    if (!SetDataListTopic(&dataList, saveTopic.instanceName, saveTopic.topicName, saveTopic.params)) {
        delete result;
        return;
    }
    dataList.len = 1;
    dataList.data = new void* [dataList.len];
    dataList.data[0] = result;
    Publish(dataList);
}

void SmcDAnalysis::Run()
{
    DetectTcpConnectionMode();
    curTime++;
    if (curTime == analysisTime) {
        PublishData();
        curTime = 0;
    }
}
}

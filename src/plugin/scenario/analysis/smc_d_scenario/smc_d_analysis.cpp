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
#include "smc_d_analysis.h"

namespace oeaware {
const int MS_PER_SEC = 1000;

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
    auto paramsMap = GetKeyValueFromString(topic.params);
    if (paramsMap.count("t")) {
        analysisTime = atoi(paramsMap["t"].data());
    }
    if (paramsMap.count("threshold1")) {
        changeRateThreshold = atof(paramsMap["threshold1"].data());
    }
    if (paramsMap.count("threshold2")) {
        netFlowThreshold = atoi(paramsMap["threshold2"].data());
    }
    beginTime = std::chrono::high_resolution_clock::now();
    isPublished = false;
    saveTopic = topic;
    return Result(OK);
}

void SmcDAnalysis::CloseTopic(const Topic &topic)
{
    (void)topic;
}

Result SmcDAnalysis::Enable(const std::string &param)
{
    (void)param;
    return Result(OK);
}

void SmcDAnalysis::Disable()
{
    prevEstablishedCount = 0;
    prevCloseWaitCount = 0;
    totalEstablishedDelta = 0;
    totalCloseWaitDelta = 0;
    curTime = 0;
    netFlow = 0;
}

void SmcDAnalysis::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

bool SmcDAnalysis::GenNlOpen()
{
    struct nl_sock *sk = nl_socket_alloc();
    if (!sk) {
        return false;
    }

    int rc = genl_connect(sk);
    if (rc) {
        nl_socket_free(sk);
        return false;
    }

    int smcId = genl_ctrl_resolve(sk, "SMC_GEN_NETLINK"); // SMC Generic Netlink Family Name
    if (smcId < 0) {
        nl_close(sk);
        nl_socket_free(sk);
        return false;
    }
    nl_close(sk);
    nl_socket_free(sk);
    return true;
}

std::string SmcDAnalysis::ExecuteCommand(const std::string &cmd)
{
    const int bufferSize = 128;
    std::array<char, bufferSize> buffer;
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

void SmcDAnalysis::GetLoNetworkFlow()
{
    std::string cmd = "sar -n DEV 1 1";
    std::string output = ExecuteCommand(cmd);
    std::istringstream stream(output);
    if (output.empty()) {
        WARN(logger, "The command 'sar -n DEV 1 1' returned no output. Check if 'sysstat' is installed.");
        netFlow = 0;
        return;
    }
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }
        if (line.find("lo") != std::string::npos) {
            std::istringstream lineStream(line);
            std::string iface;
            std::string rxStr;
            std::string txStr;
            std::string rxkBs;
            std::string txkBs;
            lineStream >> iface >> iface >> rxStr >> txStr >> rxkBs >> txkBs;
            if (iface == "lo") {
                double rx = std::stod(rxkBs);
                double tx = std::stod(txkBs);
                int kBtoMB = 2048;
                netFlow = (rx + tx) / kBtoMB; // MB/S
                break;
            }
        }
    }
}

int SmcDAnalysis::GetConnectionCount(const std::string &state)
{
    std::string cmd = "grep -E '^ *[0-9]+:' /proc/net/tcp | awk '$2 ~ /^0100007F/ && $3 ~ /^0100007F/ && $4 == \""
                    + state + "\" {count++} END {print count+0}'";
    std::string output = ExecuteCommand(cmd);
    if (output.empty()) {
        return 0;
    }
    return std::stoi(output);
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
        GetLoNetworkFlow();
        return;
    }
    totalEstablishedDelta += establishedDelta;
    totalCloseWaitDelta += closeWaitDelta;
}

void *SmcDAnalysis::GetResult()
{
    const char smcDTcpChangeRate[] = "SmcDTcpChangeRate";
    const char loNetFlow[] = "LocalNetFlow";
    const char suggestionSmcEnabled[] = "use smc-d";
    const char conclusionNoSupport[] = "The current environment does not support SMC-D.";
    const char conclusionSilent[] = "Current scenario detects fewer or no connections.Don't use smc-d.";
    const char conclusionLongConn[] = "Current scenario detects long-lived connections.";
    const char conclusionShortConn[] = "Current scenario detects short-lived connections.Don't use smc-d.";
    const char conclusionLowFlow[] = "Current scenario detects low local Flow.Don't use smc-d.";
    std::vector<int> type;
    type.emplace_back(DATA_TYPE_NETWORK);
    type.emplace_back(DATA_TYPE_NETWORK);
    AnalysisResultItem *result = new AnalysisResultItem;
    double totalEstablishedRate = 0;
    double totalCloseWaitRate = 0;
    if (prevEstablishedCount != 0) {
        totalEstablishedRate = totalEstablishedDelta / static_cast<double>(prevEstablishedCount);
        totalCloseWaitRate = totalCloseWaitDelta / static_cast<double>(prevEstablishedCount);
    }
    bool isSilence = prevEstablishedCount < 50;
    bool isHighFlow = netFlow >= netFlowThreshold;
    bool isSmcDUsable = totalEstablishedRate < changeRateThreshold && totalCloseWaitRate < changeRateThreshold;
    std::vector<std::vector<std::string>> metrics;
    std::string conclusion;
    std::vector<std::string> suggestionItem;
    if (!GenNlOpen()) {
        conclusion = conclusionNoSupport;
    } else if (isSilence) {
        metrics.emplace_back(std::vector<std::string>{
            smcDTcpChangeRate, "low", "stable"
        });
        metrics.emplace_back(std::vector<std::string>{
            loNetFlow, std::to_string(netFlow)+"(MB/s)", isHighFlow ? "high" : "low"
        });
        conclusion = conclusionSilent;
    } else if (!isHighFlow) {
        metrics.emplace_back(std::vector<std::string>{
            smcDTcpChangeRate, std::to_string(totalEstablishedRate), isSmcDUsable ? "low" : "high"
        });
        metrics.emplace_back(std::vector<std::string>{
            loNetFlow, std::to_string(netFlow)+"(MB/s)", isHighFlow ? "high" : "low"
        });
        conclusion = conclusionLowFlow;
    } else {
        metrics.emplace_back(std::vector<std::string>{
            smcDTcpChangeRate, std::to_string(totalEstablishedRate), isSmcDUsable ? "low" : "high"
        });
        metrics.emplace_back(std::vector<std::string>{
            loNetFlow, std::to_string(netFlow)+"(MB/s)", isHighFlow ? "high" : "low"
        });
        conclusion = isSmcDUsable ? conclusionLongConn : conclusionShortConn;
        suggestionItem.emplace_back(isSmcDUsable ? suggestionSmcEnabled : "");
        suggestionItem.emplace_back(isSmcDUsable ? "oeawarectl -e smc_tune" : "");
        suggestionItem.emplace_back(isSmcDUsable ? "Reduce latency and increase throughput." : "");
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
    auto now = std::chrono::system_clock::now();
    DetectTcpConnectionMode();
    curTime++;
    int curTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - beginTime).count();
    if (!isPublished && curTimeMs / MS_PER_SEC >= analysisTime) {
        PublishData();
        isPublished = true;
        curTime = 0;
    }
}
}

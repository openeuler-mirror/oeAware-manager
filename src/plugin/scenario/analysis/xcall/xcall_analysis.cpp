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
#include "xcall_analysis.h"
#include <unistd.h>
#include <oeaware/data/thread_info.h>
#include <oeaware/utils.h>
#include <oeaware/data/env_data.h>
#include "analysis_utils.h"
namespace oeaware {
void XcallAnalysis::InitSyscallTable()
{
    std::ifstream file("/usr/include/asm-generic/unistd.h");
    if (!file.is_open()) {
        WARN(logger, "syscall table init failed, /usr/include/asm-generic/unistd.h is not exsit.");
        return;
    }
    std::string line;
    static int sysCnt = 5;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string word;
        ss >> word;
        if (word != "#define") {
            continue;
        }
        ss >> word;
        if (word.size() > sysCnt && word.substr(0, sysCnt) == "__NR_") {
            int num;
            ss >> num;
            syscallTable[word.substr(sysCnt)] = num;
        }
    }
}

XcallAnalysis::XcallAnalysis()
{
    name = OE_XCALL_ANALYSIS;
    period = ANALYSIS_TIME_PERIOD;
    priority = 1;
    type = SCENARIO;
    version = "1.0.0";
    InitSyscallTable();
    subscribeTopics.emplace_back(Topic{OE_ENV_INFO, "cpu_util", ""});
    subscribeTopics.emplace_back(Topic{OE_THREAD_COLLECTOR, OE_THREAD_COLLECTOR, ""});
    for (auto &topic : topicStrs) {
        supportTopics.emplace_back(Topic{name, topic, ""});
    }
}

Result XcallAnalysis::Enable(const std::string &param)
{
    if (!oeaware::FileExist("/proc/1/xcall")) {
        return oeaware::Result(FAILED, "xcall does not open. If the system supports xcall, "
                "please add 'xcall' to cmdline.");
    }
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }
    return Result(OK);
}

void XcallAnalysis::Disable()
{
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
}

void XcallAnalysis::OutXcallConfig(int pid, const std::string &pName)
{
    std::string path = DEFAULT_PLUGIN_PATH + "/xcall" + "-" + std::to_string(pid) + ".yaml";
    std::ofstream file(path);
    if (!file.is_open()) {
        return;
    }
    std::ifstream straceFile("/tmp/strace.txt");
    std::string line;
    int skipLine = 4;
    std::vector<XcallInfo> vec;
    while (getline(straceFile, line)) {
        if (skipLine > 0) {
            skipLine--;
            continue;
        }
        if (line[0] == '-') {
            break;
        }
        double time;
        double sec;
        double usecCall;
        int calls;
        std::string err;
        std::string callName;
        std::stringstream ss(line);
        ss >> time >> sec >> usecCall >> calls >> err >> callName;
        // when the err is empty, the fifth parameter is the callName.
        if (callName.empty()) {
            callName = err;
        }
        vec.emplace_back(XcallInfo{calls, time, callName});
    }
    sort(vec.begin(), vec.end(), [](const XcallInfo &x1, const XcallInfo &x2) {
        return x1.time > x2.time;
    });
    file << pName << ":\n";
    file << "- xcall1: ";
    for (int i = 0; i < threshold && i < vec.size(); ++i) {
        if (i) file << ",";
        file << syscallTable[vec[i].callName];
    }
    file.close();
}

static void Strace(int time, int pid)
{
    std::string cmd = "timeout " + std::to_string(time) + " strace -cp " + std::to_string(pid) +
        " -o /tmp/strace.txt &";
    FILE *file = popen(cmd.data(), "r");
    if (!file) {
        return;
    }
    int status = pclose(file);
    if (status == -1) {
        return;
    }
}

void XcallAnalysis::SetTopicResult(const std::string &type, const std::string &res)
{
    std::lock_guard<std::mutex> lock(topicStatus[type].resMutex);
    topicStatus[type].res = res;
}

static void GetNameFromPid(int pid, std::string &name)
{
    std::ifstream file("/proc/" + std::to_string(pid) + "/comm");
    if (!file.is_open()) {
        return;
    }
    file >> name;
}

Result XcallAnalysis::OpenTopic(const oeaware::Topic &topic)
{
    if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
        return Result(FAILED, "topic " + topic.topicName + " not support!");
    }
    auto param = topic.params;
    auto paramsMap = GetKeyValueFromString(param);
    int time = 0;
    int pid = -1;
    if (paramsMap.count("t")) {
        time = atoi(paramsMap["t"].data());
    }
    if (paramsMap.count("pid")) {
        pid = atoi(paramsMap["pid"].data());
    }
    auto topicType = topic.GetType();
    topicStatus[topicType].open = true;
    topicStatus[topicType].pid = pid;
    topicStatus[topicType].time = time;
    topicStatus[topicType].beginTime = std::chrono::steady_clock::now();
    topicStatus[topicType].isPublish = false;
    // run strace in background
    
    if (pid != -1) {
        GetNameFromPid(pid, topicStatus[topicType].pName);
        Strace(time, pid);
    }
    return Result(OK);
}

void XcallAnalysis::CloseTopic(const oeaware::Topic &topic)
{
    auto topicType = topic.GetType();
    topicStatus.erase(topicType);
}

void XcallAnalysis::UpdateData(const DataList &dataList)
{
    std::string instanceName = dataList.topic.instanceName;
    std::string topicName = dataList.topic.topicName;
    for (auto &p : topicStatus) {
        auto topicType = p.first;
        const auto &topic = Topic::GetTopicFromType(topicType);
        if (p.second.open) {
            if (instanceName == OE_ENV_INFO) {
                auto cpuUtilData = static_cast<EnvCpuUtilParam*>(dataList.data[0]);
                auto cpuSys = cpuUtilData->times[cpuUtilData->cpuNumConfig][CPU_SYSTEM];
                auto cpuSum = cpuUtilData->times[cpuUtilData->cpuNumConfig][CPU_TIME_SUM];
                cpuSystemUsage = static_cast<double>(cpuSys) / cpuSum;
                p.second.cpuSys += cpuSystemUsage;
                p.second.cnt++;
            }
        }
    }
}

void XcallAnalysis::PublishData(const Topic &topic)
{
    DataList dataList;
    SetDataListTopic(&dataList, topic.instanceName, topic.topicName, topic.params);
    dataList.len = 1;
    dataList.data = new void *[dataList.len];
    dataList.data[0] = &analysisResultItem;
    Publish(dataList);
}

void XcallAnalysis::Run()
{
    for (auto &pair : topicStatus) {
        auto type = pair.first;
        auto &xcallTopic = pair.second;
        auto now = std::chrono::steady_clock::now();
        auto t = std::chrono::duration_cast<std::chrono::milliseconds>(now - xcallTopic.beginTime);
        int milliseconds = 1000;
        if (!xcallTopic.isPublish && t.count() >= xcallTopic.time * milliseconds) {
            xcallTopic.isPublish = true;
            Analysis(xcallTopic);
            PublishData(Topic::GetTopicFromType(type));
        }
    }
}

void XcallAnalysis::Analysis(const XcallTopic &xcallTopic)
{
    std::vector<int> type;
    std::vector<std::vector<std::string>> metrics;
    type.emplace_back(DATA_TYPE_CPU);
    double cpu = xcallTopic.cpuSys / xcallTopic.cnt * 100;
    metrics.emplace_back(std::vector<std::string>{"kernel_cpu_usage", std::to_string(cpu) + "%",
        (cpu > threshold ? "high for xcall tune" : "low for dynamic xcall tune")});
    std::string conclusion;
    std::vector<std::string> suggestionItem;
    if (cpu > threshold) {
        OutXcallConfig(xcallTopic.pid, xcallTopic.pName);
        conclusion = "For xcall_tune, the cpu_sys_usage is high. Please enable xcall_tune";
        suggestionItem.emplace_back("use xcall");
        suggestionItem.emplace_back("oeawarectl -e xcall_tune");
        suggestionItem.emplace_back("activate idle physical cores during low system loads to improveperformance");
    }
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
}
}
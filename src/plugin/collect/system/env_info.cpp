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

#include "env_info.h"
#include <numa.h>
#include <unistd.h>
#include <iostream>
#include <securec.h>
#include <dirent.h>
#include <cstdlib>
#include "oeaware/data/env_data.h"

using namespace oeaware;

bool EnvInfo::UpdateProcStat()
{
    int cpuNum = envStaticInfo.cpuNumConfig;
    const std::string path = "/proc/stat";
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    // read all file content into memory
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // solve total cpu data
    std::istringstream iss(content);
    std::string line;
    if (!std::getline(iss, line)) {
        WARN(logger, "Failed to parse total CPU data.");
        return false;
    }
    
    std::istringstream totalCpuStream(line);
    std::string label;
    totalCpuStream >> label; // read "cpu "

    for (int i = 0; i < CPU_TIME_SUM; ++i) {
        if (!(totalCpuStream >> cpuTime[cpuNum][i])) {
            WARN(logger, "Failed to parse total CPU data.");
            return false;
        }
    }

    const std::string header = "cpu";
    while (std::getline(iss, line)) {
        if (line.compare(0, header.length(), header) != 0 || !isdigit(line[header.length()])) {
            break;
        }

        std::istringstream cpuStream(line);
        std::string cpuLabel;
        int readCpu = -1;

        cpuStream >> cpuLabel;
        if (sscanf_s(cpuLabel.c_str(), "cpu%d", &readCpu) != 1 || readCpu < 0 || readCpu >= cpuNum) {
            continue;
        }

        for (int i = 0; i < CPU_TIME_SUM; ++i) {
            if (!(cpuStream >> cpuTime[readCpu][i])) {
                WARN(logger, "Failed to parse CPU " << readCpu << " data.");
                return false;
            }
        }
    }

    return true;
}

bool EnvInfo::UpdateCpuDiffTime(std::vector<std::vector<uint64_t>> oldCpuTime)
{
    bool ret = UpdateProcStat();
    for (int cpu = 0; cpu < envStaticInfo.cpuNumConfig + 1; cpu++) {
        envCpuUtilInfo.times[cpu][CPU_TIME_SUM] = 0;
        for (int type = 0; type < CPU_TIME_SUM; type++) {
            envCpuUtilInfo.times[cpu][type] = cpuTime[cpu][type] - oldCpuTime[cpu][type];
            envCpuUtilInfo.times[cpu][CPU_TIME_SUM] += envCpuUtilInfo.times[cpu][type];
        }
    }
    return ret;
}

void ResetEnvStaticInfo(EnvStaticInfo &envStaticInfo)
{
    envStaticInfo.numaNum = 0;
    envStaticInfo.cpuNumConfig = 0;
    envStaticInfo.pageSize = 0;
    envStaticInfo.pageMask = 0;
    if (envStaticInfo.cpu2Node) {
        delete[] envStaticInfo.cpu2Node;
        envStaticInfo.cpu2Node = nullptr;
    }
    if (envStaticInfo.numaDistance) {
        for (int i = 0; i < envStaticInfo.numaNum; i++) {
            if (envStaticInfo.numaDistance[i]) {
                delete[] envStaticInfo.numaDistance[i];
                envStaticInfo.numaDistance[i] = nullptr;
            }
        }
        delete[] envStaticInfo.numaDistance;
        envStaticInfo.numaDistance = nullptr;
    }
}
void ResetEnvRealTimeInfo(EnvRealTimeInfo &envRealTimeInfo)
{
    envRealTimeInfo.dataReady = ENV_DATA_NOT_READY;
    envRealTimeInfo.cpuNumOnline = 0;
    envRealTimeInfo.cpuNumConfig = 0;
    if (envRealTimeInfo.cpuOnLine != nullptr) {
        delete[] envRealTimeInfo.cpuOnLine;
        envRealTimeInfo.cpuOnLine = nullptr;
    }
}

void EnvInfo::ResetEnvCpuUtilInfo()
{
    envCpuUtilInfo.cpuNumConfig = 0;
    envCpuUtilInfo.dataReady = ENV_DATA_NOT_READY;
    for (int cpu = 0; cpu < envStaticInfo.cpuNumConfig + 1; cpu++) {
        delete[] envCpuUtilInfo.times[cpu];
        envCpuUtilInfo.times[cpu] = nullptr;
    }
    delete[] envCpuUtilInfo.times;
    envCpuUtilInfo.times = nullptr;
}

EnvInfo::EnvInfo()
{
    name = OE_ENV_INFO;
    version = "1.0.0";
    period = 1000; // 1000ms, consider reducing period in the future
    priority = 0;
    type = 0;
    for (const auto &it : topicStr) {
        oeaware::Topic topic;
        topic.instanceName = this->name;
        topic.topicName = it;
        topic.params = "";
        supportTopics.push_back(topic);
    }
    description += "[introduction] \n";
    description += "               Collect information of environment \n";
    description += "[version] \n";
    description += "         " + version + "\n";
    description += "[instance name]\n";
    description += "                 " + name + "\n";
    description += "[instance period]\n";
    description += "                 " + std::to_string(period) + " ms\n";
    description += "[provide topics]\n";
    description += "                 none\n";
    description += "[running require arch]\n";
    description += "                      aarch64 x86\n";
    description += "[running require environment]\n";
    description += "                 kernel version(4.19, 5.10, 6.6)\n";
    description += "[running require topics]\n";
    description += "                        none\n";
	description += "[provide topics]\n";
	description += "                 topicName:static, params:\"\", usage:get static environment info\n";
    description += "                 topicName:realtime, params:\"\", usage:get realtime environment info\n";
    description += "                 topicName:cpu_util, params:\"\", usage:get cpu utilization info\n";
}

oeaware::Result EnvInfo::OpenTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name) {
        return oeaware::Result(FAILED, "OpenTopic:" + topic.GetType() + " failed, instanceName is not match");
    }

    if (topic.params != "") {
        return oeaware::Result(FAILED, "OpenTopic:" + topic.GetType() + " failed, params is not empty");
    }
    for (auto &iter : topicStr) {
        if (iter != topic.topicName) {
            continue;
        }
        if (topicParams[iter].open) {
            WARN(logger, topic.GetType() << " has been opened before!");
            return oeaware::Result(OK);
        }
        topicParams[iter].open = true;
        if (topic.topicName == "realtime") {
            InitEnvRealTimeInfo();
        } else if (topic.topicName == "cpu_util") {
            InitEnvCpuUtilInfo();
        }
        return oeaware::Result(OK);
    }
    return oeaware::Result(OK);
}

void EnvInfo::CloseTopic(const oeaware::Topic &topic)
{
    if (!topicParams[topic.topicName].open) {
        WARN(logger, "CloseTopic failed, " + topic.GetType() + " not open.");
        return;
    }
    std::cout << "CloseTopic " << topic.GetType() << std::endl;
    topicParams[topic.topicName].open = false;
    if (topic.topicName == "realtime") {
        ResetEnvRealTimeInfo(envRealTimeInfo);
    }
    if (topic.topicName == "cpu_util") {
        ResetEnvCpuUtilInfo();
    }
}

void EnvInfo::UpdateData(const DataList &dataList)
{
    (void)dataList;
    return;
}

oeaware::Result EnvInfo::Enable(const std::string &param)
{
    (void)param;
    // other topic may be depend on env static info, so init env static info first
    if (!InitEnvStaticInfo()) {
        return oeaware::Result(FAILED, "Enable failed, InitEnvStaticInfo failed.");
    }
    return oeaware::Result(OK);
}

void EnvInfo::Disable()
{
    ResetEnvStaticInfo(envStaticInfo);
    return;
}

void EnvInfo::Run()
{
    for (auto &topicName : topicStr) {
        if (!topicParams[topicName].open) {
            continue;
        }
        DataList dataList;
        if (topicName == "static") {
            oeaware::SetDataListTopic(&dataList, name, topicName, "");
            dataList.len = 1;
            dataList.data = new void* [1];
            dataList.data[0] = &envStaticInfo; // not need new data
        } else if (topicName == "realtime") {
            GetEnvRealtimeInfo();
            oeaware::SetDataListTopic(&dataList, name, topicName, "");
            dataList.len = 1;
            dataList.data = new void* [1];
            dataList.data[0] = &envRealTimeInfo;
        } else if (topicName == "cpu_util") {
            GetEnvCpuUtilInfo();
            oeaware::SetDataListTopic(&dataList, name, topicName, "");
            dataList.len = 1;
            dataList.data = new void* [1];
            dataList.data[0] = &envCpuUtilInfo;
        } else {
            continue;
        }
        Publish(dataList, false);
    }
}

unsigned long GetPageMask(const int &pageSize)
{
    unsigned long pageMask;
    if (pageSize > 0) {
        pageMask = ~(static_cast<unsigned long>(pageSize) - 1);
    } else {
        pageMask = ~0xFFF;
    }

    return pageMask;
}

void EnvInfo::InitNumaDistance()
{
    int numaNum = envStaticInfo.numaNum;
    envStaticInfo.numaDistance = new int *[numaNum];
    for (int i = 0; i < numaNum; ++i) {
        envStaticInfo.numaDistance[i] = new int[numaNum];
    }

    for (int i = 0; i < numaNum; ++i) {
        for (int j = 0; j < numaNum; ++j) {
            envStaticInfo.numaDistance[i][j] = numa_distance(i, j);
        }
    }
}

bool InitCpu2Node(int numaNum, int *cpu2Node, int cpuNum)
{
    /*
    * numa_node_to_cpus or numa_node_of_cpu can't work when cpu offline
    * use /sys/devices/system/node/nodeN/cpuM instead
    */
    for (int n = 0; n < numaNum; ++n) {
        std::string basePath = "/sys/devices/system/node/node" + std::to_string(n) + "/";
        DIR *cpuDir = opendir(basePath.c_str());
        if (!cpuDir) {
            std::cerr << "Failed to open directory: " << basePath << std::endl;
            return false;
        }
        struct dirent *cpuEntry;
        const std::string prefix = "cpu";
        while ((cpuEntry = readdir(cpuDir)) != nullptr) {
            // format: cpuX
            std::string cpuDirName = cpuEntry->d_name;
            if (cpuDirName.size() <= prefix.length() || cpuDirName.compare(0, prefix.length(), "cpu") != 0 \
                || !isdigit(cpuDirName[prefix.length()])) {
                continue;
            }
            int cpuId = -1;
            try {
                cpuId = std::atoi(cpuDirName.substr(prefix.length()).c_str());
            }
            catch (...) {
                continue;
            }
            if (cpuId < 0 || cpuId >= cpuNum) {
                std::cerr << "Invalid CPU ID: " << cpuId << std::endl;
                closedir(cpuDir);
                return false;
            }
            cpu2Node[cpuId] = n;
        }
        closedir(cpuDir);
    }
    return true;
}

bool EnvInfo::InitEnvStaticInfo()
{
    envStaticInfo.numaNum = numa_num_configured_nodes();
    envStaticInfo.cpuNumConfig = sysconf(_SC_NPROCESSORS_CONF);
    if (envStaticInfo.numaNum < 0 || envStaticInfo.cpuNumConfig <= 0) {
        return false;
    }
    envStaticInfo.cpu2Node = new int[envStaticInfo.cpuNumConfig];
    InitCpu2Node(envStaticInfo.numaNum, envStaticInfo.cpu2Node, envStaticInfo.cpuNumConfig);
    envStaticInfo.pageSize = sysconf(_SC_PAGE_SIZE);
    envStaticInfo.pageMask = GetPageMask(envStaticInfo.pageSize);
    InitNumaDistance();
    return true;
}

bool EnvInfo::InitEnvRealTimeInfo()
{
    envRealTimeInfo.dataReady = ENV_DATA_NOT_READY;
    envRealTimeInfo.cpuNumConfig = envStaticInfo.cpuNumConfig;
    envRealTimeInfo.cpuNumOnline = 0;
    envRealTimeInfo.cpuOnLine = new int[envRealTimeInfo.cpuNumConfig];
    return false;
}

void EnvInfo::InitEnvCpuUtilInfo()
{
    cpuTime.clear();
    cpuTime.resize(envStaticInfo.cpuNumConfig + 1);
    for (int i = 0; i < envStaticInfo.cpuNumConfig + 1; ++i) {
        // not need use CPU_UTIL_TYPE_MAX to include CPU_TIME_SUM
        cpuTime[i].resize(CPU_TIME_SUM, 0);
    }
    envCpuUtilInfo.cpuNumConfig = envStaticInfo.cpuNumConfig;
    envCpuUtilInfo.times = new uint64_t* [envStaticInfo.cpuNumConfig + 1];
    for (int i = 0; i < envStaticInfo.cpuNumConfig + 1; ++i) {
        envCpuUtilInfo.times[i] = new uint64_t[CPU_UTIL_TYPE_MAX];
    }
    UpdateProcStat();
}

bool GetOnlineCpus(std::vector<int> &onlineCpus)
{
    std::ifstream file("/sys/devices/system/cpu/online");
    if (!file.is_open()) {
        std::cerr << "Failed to open /sys/devices/system/cpu/online" << std::endl;
        return false;
    }

    std::string line;
    std::getline(file, line);
    onlineCpus = ParseRange(line);
    return true;
}

void EnvInfo::GetEnvRealtimeInfo()
{
    envRealTimeInfo.cpuNumOnline = sysconf(_SC_NPROCESSORS_ONLN);
    if (envRealTimeInfo.cpuNumOnline <= 0 || envRealTimeInfo.cpuNumOnline > envStaticInfo.cpuNumConfig) {
        envRealTimeInfo.dataReady = ENV_DATA_NOT_READY;
        return;
    }
    std::vector<int> onlineCpus;
    if (!GetOnlineCpus(onlineCpus)) {
        envRealTimeInfo.dataReady = ENV_DATA_NOT_READY;
        return;
    }
    for (int i = 0; i < envRealTimeInfo.cpuNumConfig; ++i) {
        envRealTimeInfo.cpuOnLine[i] = 0;
    }
    for (const auto &cpu : onlineCpus) {
        envRealTimeInfo.cpuOnLine[cpu] = 1;
    }
    envRealTimeInfo.dataReady = ENV_DATA_READY;
}

void EnvInfo::GetEnvCpuUtilInfo()
{
    envCpuUtilInfo.dataReady = ENV_DATA_NOT_READY;
    if (UpdateCpuDiffTime(cpuTime)) {
        envCpuUtilInfo.dataReady = ENV_DATA_READY;
    }
}

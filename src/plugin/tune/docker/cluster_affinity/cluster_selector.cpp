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
#include "cluster_selector.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <climits>
#include <cfloat>
#include <dirent.h>
#include "topology.h"

ClusterSelector::ClusterSelector() 
{
    clusterTopology = Topology();
}

bool ClusterSelector::Init(log4cplus::Logger &logger)
{
    DIR *dir;
    struct dirent *entry;
    this->logger = logger;

    if ((dir = opendir("/sys/devices/")) != nullptr) {
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name.find("l3c") != std::string::npos) {
                l3cNameList.push_back(name);
            }
        }
        closedir(dir);
    }
    
    if (l3cNameList.empty()) {
        WARN(logger, "No valid l3c file found in directory.");
        return false;
    }
    std::sort(l3cNameList.begin(), l3cNameList.end());

    int clusterId;
    for (std::string name : l3cNameList) {
        std::ifstream l3cCpumaskFile("/sys/devices/" + name + "/cpumask");
        if (!l3cCpumaskFile.is_open()) {
            WARN(logger, "/sys/devices/" + name + "/cpumask open failed.");
        }
        l3cCpumaskFile >> clusterId;
        clusterId /= clusterTopology.GetOneClusterCpuNumber();
        clusterToL3c[clusterId] = name;
    }
    return true;
}

int ClusterSelector::SelectIdleNode(const std::vector<int> &used)
{
    int nodeNum = clusterTopology.GetSystemNumaNodeNum();
    std::vector<double> dataVec(nodeNum);

    for (int i = 0; i < nodeNum; ++i) {
        if (used[i]) {
            continue;
        }
        std::vector<int> cpuList = clusterTopology.GetCpuTopology(i);
        for (int cpu : cpuList) {
            dataVec[i] += cpuLoadMap[cpu];
        }
    }

    double minVal = DBL_MAX;
    int node = -1;
    for (size_t i = 0; i < dataVec.size() && i < used.size(); ++i) {
        if (minVal > dataVec[i] && !used[i]) {
            minVal = dataVec[i];
            node = i;
        }
    }

    return node;
}

int ClusterSelector::SelectIdleCluster(int node, const std::vector<int> &used)
{
    int nodeNum = clusterTopology.GetSystemNumaNodeNum();
    if (node < 0 || node >= nodeNum) {
        ERROR(logger, "Error nodeId " << node);
        return -1;
    }

    if (nodeNum <= 0) {
        ERROR(logger, "Error nodeNum " << nodeNum);
        return -1;
    }

    int clusterNum = clusterTopology.GetClusterNumber();
    uint32_t numaClusterNum = clusterNum / nodeNum;
    uint32_t clusterIndexBegin = numaClusterNum * node;
    uint32_t clusterIndexEnd = clusterIndexBegin + numaClusterNum - 1;
    uint64_t minVal = UINT64_MAX;
    int cluster = -1;

    for (auto i = clusterIndexBegin; i <= clusterIndexEnd && (i - clusterIndexBegin) < used.size(); ++i) {
        std::string l3cName = clusterToL3c[i];
        if (minVal > scclL3cSum[l3cName] && !used[i - clusterIndexBegin]) {
            minVal = scclL3cSum[l3cName];
            cluster = i;
        }
    }

    return cluster;
}

int ClusterSelector::SelectIdleCpuFromCluster(int clusterId, const std::vector<int> &used)
{
    size_t clusterCpuNum = clusterTopology.GetOneClusterCpuNumber();
    int clusterCpuStart = clusterId * clusterCpuNum;
    int selectCpu = clusterCpuStart;
    double minVal = 100.0;
    
    for (size_t i = 0; i < used.size(); ++i) {
        if (used[i]) {
            continue;
        }
        if (cpuLoadMap[clusterCpuStart + i] < minVal) {
            minVal = cpuLoadMap[clusterCpuStart + i];
            selectCpu = clusterCpuStart + i;
        }
    }
    return selectCpu;
}

void ClusterSelector::Update(const DataList &datalist)
{
    auto l3cData = static_cast<PmuL3cData*>(datalist.data[0]);

    for (auto i = 0; i < l3cData->len; ++i) {
        auto count = l3cData->pmuData[i].count;
        SetL3cCount(count, i);
    }

    UpdateCpuLoad();
}

bool ClusterSelector::ReadCpuStats(std::map<int, CpuStats>& cpuStatsMap)
{
    std::ifstream file("/proc/stat");
    if (file.is_open() == false) {
        return false;
    }
    std::string line;

    while (getline(file, line)) {
        if (line.substr(0, 3) == "cpu") {
            std::istringstream iss(line);
            std::string cpuLabel;
            int cpuNumber = -1;
            CpuStats data;
            iss >> cpuLabel;

            if (cpuLabel.size() > 3) {
                try {
                    cpuNumber = std::stoi(cpuLabel.substr(3));
                } catch(...) {
                    continue;
                }
            }

            if (cpuNumber >= 0) {
                iss >> data.user >> data.nice >> data.system >> data.idle
                    >> data.iowait >> data.irq >> data.softirq;
                cpuStatsMap[cpuNumber] = data;
            }
        }
    }
    return true;
}

double ClusterSelector::CalculateCpuLoad(const CpuStats& prev, const CpuStats& curr)
{
    unsigned long prevTotal = prev.user + prev.nice + prev.system + prev.idle + prev.iowait + prev.irq + prev.softirq;
    unsigned long currTotal = curr.user + curr.nice + curr.system + curr.idle + curr.iowait + curr.irq + curr.softirq;

    unsigned long prevIdle = prev.idle + prev.iowait;
    unsigned long currIdle = curr.idle + curr.iowait;

    double totalDelta = currTotal - prevTotal;
    double idleDelta = currIdle - prevIdle;

    return (1.0 - (idleDelta / totalDelta)) * 100.0;
}

void ClusterSelector::UpdateCpuLoad()
{
    std::map<int, CpuStats> currentCpuStatsMap;
    ReadCpuStats(currentCpuStatsMap);

    for (const auto &pair : prevCpuStatsMap) {
        int cpuNumber = pair.first;
        double load = CalculateCpuLoad(prevCpuStatsMap[cpuNumber], currentCpuStatsMap[cpuNumber]);
        cpuLoadMap[cpuNumber] = load;
    }
    prevCpuStatsMap = currentCpuStatsMap;
}

void ClusterSelector::SetL3cCount(uint64_t count, size_t index)
{
    std::string name = l3cNameList[index];
    scclL3c[name].push(count);
    scclL3cSum[name] += count;
    if (scclL3c[name].size() > L3C_HIT_QUEUE_LENGTH) {
        scclL3cSum[name] -= scclL3c[name].front();
        scclL3c[name].pop();
    }
}
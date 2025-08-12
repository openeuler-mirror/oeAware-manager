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
#include "topology.h"
#include <fstream>
#include <sstream>
#include <string>
#include <numa.h>

Topology::Topology()
{
    clusterNum = 0;
    oneClusterCpuNum = 0;
    isHyperThreading = false;
    isSystemOfflineCpu = false;
    numaNodeNum = numa_num_configured_nodes();
    cpuTopology.resize(numaNodeNum);

    ProcessCpuTopology();
}

void Topology::AddCpuRange(int node, int start, int end)
{
    for (int cpu = start; cpu <= end; ++cpu) {
        cpuTopology[node].push_back(cpu);
    }
}

int Topology::ProcessCpuTopology()
{
    int start;
    int end;
    std::ifstream clusterCpuListFile("/sys/devices/system/cpu/cpu0/topology/cluster_cpus_list");
    std::ifstream offlineCpuFile("/sys/devices/system/cpu/offline");
    if (clusterCpuListFile.is_open()) {
        std::string clusterCpuList;
        clusterCpuListFile >> clusterCpuList;
        size_t dashPos = clusterCpuList.find('-');
        start = std::stoi(clusterCpuList.substr(0, dashPos));
        end = std::stoi(clusterCpuList.substr(dashPos + 1));
        oneClusterCpuNum = end - start + 1;
    }
    if (offlineCpuFile.is_open()) {
        std::string offlineCpu;
        offlineCpuFile >> offlineCpu;
        isSystemOfflineCpu = offlineCpu.empty() ? false : true;
    }

    int cpuNum = 0;
    std::string cpuListStr;
    for (int i = 0; i < numaNodeNum; ++i) {
        std::string path = "/sys/devices/system/node/node" + std::to_string(i) + "/cpulist";
        std::ifstream ifs(path);

        if (!ifs.is_open()) {
            return -1;
        }

        ifs >> cpuListStr;
        ifs.close();
        std::stringstream ss(cpuListStr);
        std::string range;

        while (std::getline(ss, range, ',')) {
            size_t dashPos = range.find('-');
            if (dashPos == std::string::npos) {
                cpuTopology[i].push_back(std::stoi(range));
                cpuNum++;
            } else {
                start = std::stoi(range.substr(0, dashPos));
                end = std::stoi(range.substr(dashPos + 1));
                cpuNum += (end - start + 1);
                AddCpuRange(i, start, end);
            }
        }
    }
    clusterNum = oneClusterCpuNum ? (cpuNum / oneClusterCpuNum) : 0;

    return 0;
}

int Topology::GetSystemNumaNodeNum()
{
    return numaNodeNum;
}

int Topology::GetClusterNumber()
{
    return clusterNum;
}

int Topology::GetOneClusterCpuNumber()
{
    return oneClusterCpuNum;
}

std::vector<std::vector<int>> Topology::GetCpuTopology()
{
    return cpuTopology;
}

std::vector<int> Topology::GetCpuTopology(int node)
{
    if (node < 0 || node >= numaNodeNum) {
        return std::vector<int>();
    }
    return cpuTopology[node];
}

bool Topology::IsSystemOfflineCpu()
{
    return isSystemOfflineCpu;
}
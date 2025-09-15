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
#ifndef CLUSTER_SELECTOR_H
#define CLUSTER_SELECTOR_H

#include <map>
#include <vector>
#include <string>
#include <queue>
#include "topology.h"
#include "oeaware/interface.h"
#include "oeaware/data_list.h"
#include "oeaware/data/pmu_l3c_data.h"

struct CpuStats {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
};

class ClusterSelector {
public:
    ClusterSelector();
    bool Init(log4cplus::Logger &logger);
    void Update(const DataList &datalist);

    int SelectIdleNode(const std::vector<int> &used);
    int SelectIdleCluster(int node, const std::vector<int> &used);
    int SelectIdleCpuFromCluster(int clusterId, const std::vector<int> &used);

private:
    bool ReadCpuStats(std::map<int, CpuStats>& cpuStatsMap);
    double CalculateCpuLoad(const CpuStats& prev, const CpuStats& curr);
    void UpdateCpuLoad();
    void SetL3cCount(uint64_t count, size_t index);

private:
    log4cplus::Logger logger;
    Topology clusterTopology;
    std::vector<std::string> l3cNameList;
    std::map<int, std::string> clusterToL3c;
    std::map<std::string, std::queue<uint64_t>> scclL3c;
    std::map<std::string, uint64_t> scclL3cSum;
    std::map<int, double> cpuLoadMap;
    std::map<int, CpuStats> prevCpuStatsMap;
    // Record last 10s l3c_hit count
    static constexpr int L3C_HIT_QUEUE_LENGTH = 10;
};

#endif
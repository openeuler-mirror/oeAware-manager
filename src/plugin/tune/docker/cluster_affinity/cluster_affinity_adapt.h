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
#ifndef CLUSTER_AFFINITY_ADAPT_H
#define CLUSTER_AFFINITY_ADAPT_H
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include "topology.h"
#include "cluster_selector.h"
#include "oeaware/interface.h"
#include "oeaware/data_list.h"
#include "oeaware/data/docker_data.h"

struct DockerResourceInfo {
    bool inited;
    bool optimizeBinded;
    bool dockerLended;

    std::string dockerId;
    std::queue<double> cpuUsageQueue;
    double cpuUsageSum;
    size_t cpuCores;
    std::vector<int> initialCpuSet;
    std::vector<int> initialMemSet;
    std::vector<int> currentCpuSet;
    std::vector<int> currentMemSet;
    std::unordered_map<std::string, double> borrowDockerMap;

    double initialCpuLimit;
    double lendCpuLimit;
    double currentCpuLimit;

    std::vector<int> bindClusters;
};

struct Cluster {
    int clusterId;
    std::vector<int> logicalCpuCores;
    std::vector<int> freeCpuCores;
    std::vector<int> occupiedCpuCores;
};

struct NumaClusterInfo {
    int numaId;
    std::vector<int> freeInfoVec;
    std::vector<int> occupiedInfoVec;
};

class ClusterAffinityAdapt : public oeaware::Interface {
public:
    ClusterAffinityAdapt();
    ~ClusterAffinityAdapt() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
private:
    bool Init();
    void Update(const DataList &datalist);
    oeaware::Result ReadConfig(const std::string &path);
    void Tune();
    void Exit();
    bool InitSystemClusters();
    std::string ConvertCpusetToString(std::vector<int>& nums);
    bool RecoverInitialBindStatus(const std::string& dockerId);
    bool DockerAddCpuLimitComm(const std::string& dockerId, double limit);
    bool DockerMinusCpuLimitComm(const std::string& dockerId, double limit);
    void TryBorrowCpuFromIdleDocker();
    void TryReturnCpuToIdleDocker();
    std::vector<int> TransformStrToVec(const std::string& resourceListStr);
    int SelectBindingNuma(const std::string& dockerId);
    bool DockerBindClusters(const std::string& dockerId, int nid, int clusterNum);
    bool BindCpusAndMems(const std::string& dockerId, std::vector<int> cSet, std::vector<int> mSet);
    void DockerOptimizeBindSchedule();
    int ExecuteCMD(const std::string& cmd, std::string& result);
private:
    mutable std::mutex mtx;
    Topology clusterTopology;
    ClusterSelector clusterSelector;
    std::vector<oeaware::Topic> subscribeTopics;
    bool dockerBorrowSwitch;
    bool isFirstUpdate;
    bool isSelectorInited;
    size_t systemCpuCount;
    std::unordered_map<std::string, Container> collectorPre;
    std::unordered_map<std::string, DockerResourceInfo> dockerInfos;
    std::unordered_map<int, Cluster> systemClusterInfo;
    std::unordered_map<int, NumaClusterInfo> numaClusterInfo;
    std::vector<std::vector<int>> cpuTopology;
    std::string configPath = oeaware::DEFAULT_PLUGIN_CONFIG_PATH + "/cluster_affinity.yaml";
    // Record last 30s cpu usage
    static constexpr int CPU_USAGE_QUEUE_LENGTH = 30;
};
#endif // CLUSTER_AFFINITY_ADAPT_H
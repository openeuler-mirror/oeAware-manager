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
#include "cluster_affinity_adapt.h"
#include <yaml-cpp/yaml.h>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "oeaware/utils.h"

constexpr int PERIOD = 1000;
constexpr int PRIORITY = 2;

ClusterAffinityAdapt::ClusterAffinityAdapt()
{
    name = OE_DOCKER_CLUSTER_AFFINITY_TUNE;
    description = "";
    version = "1.0.0";
    period = PERIOD;
    priority = PRIORITY;
    type = oeaware::TUNE;

    isFirstUpdate = true;
    isSelectorInited = false;
    dockerBorrowSwitch = false;
    systemCpuCount = 0;
    clusterTopology = Topology();
    clusterSelector = ClusterSelector();
    cpuTopology = clusterTopology.GetCpuTopology();
    for (const auto &vec : cpuTopology) {
        systemCpuCount += vec.size();
    }

    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = this->name;
    supportTopics.push_back(topic);
    subscribeTopics.emplace_back(oeaware::Topic{OE_PMU_L3C_COLLECTOR, "l3c", ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_DOCKER_COLLECTOR, OE_DOCKER_COLLECTOR, ""});
}

oeaware::Result ClusterAffinityAdapt::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void ClusterAffinityAdapt::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void ClusterAffinityAdapt::UpdateData(const DataList &dataList)
{
    auto instance_name = dataList.topic.instanceName;
    auto topic_name = dataList.topic.topicName;
    if (instance_name == std::string(OE_PMU_L3C_COLLECTOR) && topic_name == std::string("l3c")) {
        clusterSelector.Update(dataList);
    } else if (instance_name == std::string(OE_DOCKER_COLLECTOR) && topic_name == std::string(OE_DOCKER_COLLECTOR)) {
        this->Update(dataList);
    }
}

oeaware::Result ClusterAffinityAdapt::ReadConfig(const std::string &path)
{
    if (!oeaware::FileExist(path)) {
        return oeaware::Result(FAILED, "config file does not exist, {path:" + path + "}.");
    }
    try {
        YAML::Node node = YAML::LoadFile(path);
        if (node["docker_borrow_switch"]) {
            std::string switchStr = node["docker_borrow_switch"].as<std::string>();
            dockerBorrowSwitch = ((switchStr == "on") ? true : false);
        }
    } catch (const YAML::Exception &e) {
        return oeaware::Result(FAILED, e.what());
    }
    return oeaware::Result(OK);
}

oeaware::Result ClusterAffinityAdapt::Enable(const std::string &param)
{
    (void)param;
    auto ret = ReadConfig(configPath);
    if (ret.code != OK) {
        return ret;
    }
    if (!Init()) {
        return oeaware::Result(FAILED, "the system does not support docker cluster affinity tune.");
    }
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }
    return oeaware::Result(OK);
}

void ClusterAffinityAdapt::Disable()
{
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    Exit();
}

void ClusterAffinityAdapt::Run()
{
    Tune();
}

bool ClusterAffinityAdapt::InitSystemClusters()
{
    int clusterCpuNum = clusterTopology.GetOneClusterCpuNumber();
    if (clusterCpuNum == 0) {
        WARN(logger, "System don't support cluster architecture.");
        return false;
    }

    for (size_t i = 0; i < cpuTopology.size(); ++i) {
        int clusterNum = cpuTopology[i].size() / clusterCpuNum;
        numaClusterInfo[i].numaId = i;
        for (int j = 0; j < clusterNum; ++j) {
            int start = j * clusterCpuNum;
            int end = (j + 1) * clusterCpuNum;
            int sep = oeaware::IsSmtEnable() ? 2 : 1;
            Cluster cluster;
            cluster.clusterId = cpuTopology[i][start] / clusterCpuNum;
            /* Not support offline cpu */
            for (int k = start; k < end; k += sep) {
                cluster.freeCpuCores.push_back(cpuTopology[i][k]);
            }
            for (int k = start; k < end; k++) {
                if (std::find(cluster.freeCpuCores.begin(), cluster.freeCpuCores.end(), cpuTopology[i][k]) ==
                    cluster.freeCpuCores.end()) {
                    cluster.logicalCpuCores.push_back(cpuTopology[i][k]);
                }
            }
            systemClusterInfo[cluster.clusterId] = cluster;
            numaClusterInfo[i].freeInfoVec.push_back(cluster.clusterId);
        }
    }
    return true;
}

bool ClusterAffinityAdapt::Init()
{
    if (!InitSystemClusters()) {
        ERROR(logger, "InitSystemClusters. System dont't support cluster architecture.");
        return false;
    }
    if (clusterTopology.IsSystemOfflineCpu()) {
        ERROR(logger, "System exit offline cpu. not support docker cluster affinity tune.");
        return false;
    }
    if (isSelectorInited) {
        return true;
    }
    if (!clusterSelector.Init(logger)) {
        ERROR(logger, "Cluster Selector Init failed.");
        return false;
    }
    isSelectorInited = true;
    return true;
}

void ClusterAffinityAdapt::Update(const DataList& datalist)
{
    std::lock_guard<std::mutex> lock(mtx);
    std::unordered_map<std::string, Container> collector;
    for (uint64_t i = 0; i < datalist.len; i++) {
        auto *tmp = static_cast<Container*>(datalist.data[i]);
        collector[tmp->id] = *tmp;
    }

    if (isFirstUpdate) {
        collectorPre = collector;
        isFirstUpdate = false;
        return;
    }

    for (auto it = dockerInfos.begin(); it != dockerInfos.end();) {
        if (collector.count(it->first) == 0) {
            RecoverInitialBindStatus(it->first);
            it = dockerInfos.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto &p : collector) {
        if (collectorPre.count(p.first) == 0) {
            continue;
        }

        if (dockerInfos[p.first].inited == false) {
            dockerInfos[p.first].dockerId = p.first;
            dockerInfos[p.first].initialCpuLimit = (p.second.cfs_period_us > 0) ? 
                (p.second.cfs_quota_us / p.second.cfs_period_us) : 0;
            dockerInfos[p.first].currentCpuLimit = dockerInfos[p.first].initialCpuLimit;
            dockerInfos[p.first].lendCpuLimit = 0;
            dockerInfos[p.first].initialCpuSet = TransformStrToVec(p.second.cpus);
            dockerInfos[p.first].initialMemSet = TransformStrToVec(p.second.mems);
            dockerInfos[p.first].currentCpuSet = dockerInfos[p.first].initialCpuSet;
            dockerInfos[p.first].currentMemSet = dockerInfos[p.first].initialMemSet;
            dockerInfos[p.first].cpuUsageSum = 0;
            dockerInfos[p.first].inited = true;
            dockerInfos[p.first].optimizeBinded = false;
            dockerInfos[p.first].dockerLended = false;
        }

        // Real time data calculate
        uint64_t dockerCpuUsedNs = p.second.cpu_usage - collectorPre[p.first].cpu_usage;
        uint64_t systemCpuUsedNs = (p.second.system_cpu_usage - collectorPre[p.first].system_cpu_usage) * 10000000; // s to ns
        double cpuUtilization = ((static_cast<double>(dockerCpuUsedNs) / static_cast<double>(systemCpuUsedNs)) * 
            systemCpuCount) * 100;
        dockerInfos[p.first].cpuUsageQueue.push(cpuUtilization);
        dockerInfos[p.first].cpuUsageSum += cpuUtilization;
        if (dockerInfos[p.first].cpuUsageQueue.size() > CPU_USAGE_QUEUE_LENGTH) {
            dockerInfos[p.first].cpuUsageSum -= dockerInfos[p.first].cpuUsageQueue.front();
            dockerInfos[p.first].cpuUsageQueue.pop();
        }
        dockerInfos[p.first].cpuCores = std::max(1, static_cast<int>(std::ceil(cpuUtilization / 100.0)));
    }
    collectorPre = collector;
}

void ClusterAffinityAdapt::Tune()
{
    DockerOptimizeBindSchedule();
    if (dockerBorrowSwitch) {
        TryBorrowCpuFromIdleDocker();
        TryReturnCpuToIdleDocker();
    }
}

void ClusterAffinityAdapt::Exit()
{
    std::lock_guard<std::mutex> lock(mtx);
    for (auto &p : dockerInfos) {
        if (RecoverInitialBindStatus(p.first) == false) {
            ERROR(logger, p.first << " reset initial bind status failed.");
        }
    }
    isFirstUpdate = true;
    dockerInfos.clear();
    collectorPre.clear();
    systemClusterInfo.clear();
    numaClusterInfo.clear();
}

bool ClusterAffinityAdapt::RecoverInitialBindStatus(const std::string& dockerId)
{
    if (dockerInfos[dockerId].optimizeBinded == false) {
        return true;
    }

    std::string cpusetStr = ConvertCpusetToString(dockerInfos[dockerId].initialCpuSet);
    std::string memorysetStr = ConvertCpusetToString(dockerInfos[dockerId].initialMemSet);
    std::string cpusStr = std::to_string(dockerInfos[dockerId].initialCpuLimit);
    std::string updateCpusCmd = "docker update --cpus=" + cpusStr + " " + dockerId;
    std::string updateCpuSetCmd = "docker update --cpuset-cpus=" + cpusetStr + " " + dockerId;
    std::string updateMemSetCmd = "docker update --cpuset-mems=" + memorysetStr + " " + dockerId;
    std::string updateCpusResult = "";
    std::string updateCpuSetResult = "";
    std::string updateMemSetResult = "";
    if (ExecuteCMD(updateCpusCmd, updateCpusResult) < 0 ||
        ExecuteCMD(updateCpuSetCmd, updateCpuSetResult) < 0 ||
        ExecuteCMD(updateMemSetCmd, updateMemSetResult) < 0) {
        ERROR(logger, "Recover docker " << dockerId << " initial bind status failed.");
        return false;
    }

    for (int nid : dockerInfos[dockerId].currentMemSet) {
        std::vector<int> tmp;
        for (int clusterId : numaClusterInfo[nid].occupiedInfoVec) {
            if (std::find(dockerInfos[dockerId].bindClusters.begin(),
                          dockerInfos[dockerId].bindClusters.end(),
                          clusterId) != dockerInfos[dockerId].bindClusters.end()) {
                numaClusterInfo[nid].freeInfoVec.push_back(clusterId);
                continue;
            }
            tmp.push_back(clusterId);
        }
        numaClusterInfo[nid].occupiedInfoVec = tmp;
    }

    size_t clusterCpuNum = clusterTopology.GetOneClusterCpuNumber();
    if (clusterCpuNum == 0) {
        WARN(logger, "System don't support cluster architecture.");
        return false;
    }
    std::map<int, std::vector<int>> freeCores;
    for (int cpu : dockerInfos[dockerId].currentCpuSet) {
        int clusterId = cpu / clusterCpuNum;
        freeCores[clusterId].push_back(cpu);
        systemClusterInfo[clusterId].freeCpuCores.push_back(cpu);
    }
    for (const auto &p : freeCores) {
        int clusterId = p.first;
        std::vector<int> tmp;
        for (int cpu : systemClusterInfo[clusterId].occupiedCpuCores) {
            if (std::find(p.second.begin(), p.second.end(), cpu) != p.second.end()) {
                tmp.push_back(cpu);
            }
        }
        systemClusterInfo[clusterId].occupiedCpuCores = tmp;
    }

    for (auto &p : dockerInfos) {
        if (p.second.borrowDockerMap.count(dockerId) == 0) {
            continue;
        }
        if (DockerMinusCpuLimitComm(p.first, p.second.borrowDockerMap[dockerId]) == false) {
            ERROR(logger, "docker " << p.first << " return cpu to idle docker failed.");
        }
    }
    INFO(logger, "Docker " << dockerId << " recover initial bind status successed.");

    return true;
}

bool ClusterAffinityAdapt::DockerAddCpuLimitComm(const std::string& dockerId, double limit)
{
    int nid = dockerInfos[dockerId].currentMemSet[0];
    size_t needBorrowCount = static_cast<size_t>(std::ceil(dockerInfos[dockerId].currentCpuLimit + limit) -
        std::ceil(dockerInfos[dockerId].currentCpuLimit));
    size_t clusterCpuNum = clusterTopology.GetOneClusterCpuNumber();
    int clusterId = dockerInfos[dockerId].bindClusters.back();
    size_t clusterRemainCpus = systemClusterInfo[clusterId].freeCpuCores.size();
    std::vector<int> addCpuSet;
    std::vector<int> addBindCluster;

    if (clusterCpuNum == 0) {
        WARN(logger, "System don't support cluster architecture.");
        return false;
    }

    // If docker bind cluster have remain cpu
    if (clusterRemainCpus >= needBorrowCount) {
        std::vector<int> used(clusterCpuNum, 1);
        int clusterStartCpu = clusterId * clusterCpuNum;
        for (int cpu : systemClusterInfo[clusterId].freeCpuCores) {
            used[cpu - clusterStartCpu] = 0;
        }
        // when cluster is full, no need to select
        for (size_t i = 0; i < needBorrowCount; ++i) {
            int cpu = clusterSelector.SelectIdleCpuFromCluster(clusterId, used);
            used[cpu - clusterStartCpu] = 1;
            systemClusterInfo[clusterId].occupiedCpuCores.push_back(cpu);
            addCpuSet.push_back(cpu);
        }
        std::vector<int> tmp;
        for (int cpu : systemClusterInfo[clusterId].freeCpuCores) {
            if (std::find(addCpuSet.begin(), addCpuSet.end(), cpu) == addCpuSet.end()) {
                tmp.push_back(cpu);
            }
        }
        systemClusterInfo[clusterId].freeCpuCores = tmp;
        needBorrowCount = 0; 
    } else {
        addCpuSet = systemClusterInfo[clusterId].freeCpuCores;
        systemClusterInfo[clusterId].occupiedCpuCores.insert(
            systemClusterInfo[clusterId].occupiedCpuCores.end(),
            systemClusterInfo[clusterId].freeCpuCores.begin(),
            systemClusterInfo[clusterId].freeCpuCores.end());
        systemClusterInfo[clusterId].freeCpuCores.clear();
        needBorrowCount -= clusterRemainCpus;
    }

    if (needBorrowCount > 0) {
        int sep = oeaware::IsSmtEnable() ? 2 : 1;
        size_t needClusterNum = 
            static_cast<size_t>(std::ceil(static_cast<double>(needBorrowCount) / (clusterCpuNum / sep)));
        int numaClusterNum = cpuTopology[nid].size() / clusterCpuNum;
        int clusterBeginIndex = cpuTopology[nid][0] / clusterCpuNum;
        std::vector<int> used(numaClusterNum, 0);
        for (int clusterId : numaClusterInfo[nid].occupiedInfoVec) {
            int usedIndex = clusterId - clusterBeginIndex;
            used[usedIndex] = 1;
        }

        // Select idle cluster by l3c collect pmu info
        for (size_t i = 0; i < needClusterNum; ++i) {
            clusterId = clusterSelector.SelectIdleCluster(nid, used);
            used[clusterId - clusterBeginIndex] = 1;
            numaClusterInfo[nid].occupiedInfoVec.push_back(clusterId);
            addBindCluster.push_back(clusterId);
        }

        // Refresh freeCluster
        std::vector<int> tmpFreeInfoVec;
        for (int clusterId : numaClusterInfo[nid].freeInfoVec) {
            if (std::find(numaClusterInfo[nid].occupiedInfoVec.begin(),
                          numaClusterInfo[nid].occupiedInfoVec.end(),
                          clusterId) == numaClusterInfo[nid].occupiedInfoVec.end()) {
                tmpFreeInfoVec.push_back(clusterId);
            }
        }
        numaClusterInfo[nid].freeInfoVec = tmpFreeInfoVec;

        for (size_t i = 0; i < addBindCluster.size() - 1; i++) {
            addCpuSet.insert(addCpuSet.end(),
                systemClusterInfo[addBindCluster[i]].freeCpuCores.begin(),
                systemClusterInfo[addBindCluster[i]].freeCpuCores.end());
            systemClusterInfo[addBindCluster[i]].occupiedCpuCores.insert(
                systemClusterInfo[addBindCluster[i]].occupiedCpuCores.end(),
                systemClusterInfo[addBindCluster[i]].freeCpuCores.begin(),
                systemClusterInfo[addBindCluster[i]].freeCpuCores.end());
            needBorrowCount -= systemClusterInfo[addBindCluster[i]].freeCpuCores.size();
            systemClusterInfo[addBindCluster[i]].freeCpuCores.clear();
        }
        clusterId = addBindCluster.back();
        std::vector<int> cpuUsed(clusterCpuNum, 1);
        int clusterCpuStart = clusterId * clusterCpuNum;
        for (int cpu : systemClusterInfo[clusterId].freeCpuCores) {
            cpuUsed[cpu - clusterCpuStart] = 0;
        }
        for (size_t i = 0; i < needBorrowCount; ++i) {
            int cpu = clusterSelector.SelectIdleCpuFromCluster(clusterId, cpuUsed);
            cpuUsed[cpu - clusterCpuStart] = 1;
            systemClusterInfo[clusterId].occupiedCpuCores.push_back(cpu);
            addCpuSet.push_back(cpu);
        }
        std::vector<int> tmp;
        for (int cpu : systemClusterInfo[clusterId].freeCpuCores) {
            if (std::find(addCpuSet.begin(), addCpuSet.end(), cpu) == addCpuSet.end()) {
                tmp.push_back(cpu);
            }
        }
        systemClusterInfo[clusterId].freeCpuCores = tmp;
        needBorrowCount = 0;
    }
    dockerInfos[dockerId].currentCpuSet.insert(dockerInfos[dockerId].currentCpuSet.end(),
        addCpuSet.begin(), addCpuSet.end());
    dockerInfos[dockerId].bindClusters.insert(dockerInfos[dockerId].bindClusters.end(),
        addBindCluster.begin(), addBindCluster.end());

    std::string cpusetStr = ConvertCpusetToString(dockerInfos[dockerId].currentCpuSet);
    dockerInfos[dockerId].currentCpuLimit += limit;
    std::string cpusStr = std::to_string(dockerInfos[dockerId].currentCpuLimit);
    // refresh cpuset
    std::string updateCpusCmd = "docker update --cpus=" + cpusStr + " " + dockerId;
    std::string updateCpuSetCmd = "docker update --cpuset-cpus=" + cpusetStr + " " + dockerId;
    std::string updateCpusResult = "";
    std::string updateCpuSetResult = "";
    if (ExecuteCMD(updateCpusCmd, updateCpusResult) < 0 ||
        ExecuteCMD(updateCpuSetCmd, updateCpuSetResult) < 0) {
        ERROR(logger, "Docker " << dockerId << " update cpuset-cpus and cpus failed.");
        return false;
    }
    INFO(logger, "Successfully update docker " << dockerId << " with CPUs: " << cpusetStr);
    INFO(logger, "docker " << dockerId << " add " << limit << " cpus, current cpulimit is " << 
        dockerInfos[dockerId].currentCpuLimit);
    return true;
}

bool ClusterAffinityAdapt::DockerMinusCpuLimitComm(const std::string& dockerId, double limit)
{
    int nid = dockerInfos[dockerId].currentMemSet[0];
    size_t needReturnCount = static_cast<size_t>(std::ceil(dockerInfos[dockerId].currentCpuLimit) -
        std::ceil(dockerInfos[dockerId].currentCpuLimit - limit));
    int clusterId = dockerInfos[dockerId].bindClusters.back();
    size_t clusterOccupiedCpus = systemClusterInfo[clusterId].occupiedCpuCores.size();
    std::vector<int> minusCpuSet;

    if (needReturnCount <= clusterOccupiedCpus) {
        for (size_t i = 0; i < needReturnCount; ++i) {
            minusCpuSet.push_back(systemClusterInfo[clusterId].occupiedCpuCores.back());
            systemClusterInfo[clusterId].freeCpuCores.push_back(systemClusterInfo[clusterId].occupiedCpuCores.back());
            systemClusterInfo[clusterId].occupiedCpuCores.pop_back();
        }
        needReturnCount = 0;
    } else {
        while (needReturnCount > 0) {
            clusterId = dockerInfos[dockerId].bindClusters.back();
            if (systemClusterInfo[clusterId].occupiedCpuCores.empty()) {
                numaClusterInfo[nid].freeInfoVec.push_back(dockerInfos[dockerId].bindClusters.back());
                numaClusterInfo[nid].occupiedInfoVec.erase(std::remove(
                    numaClusterInfo[nid].occupiedInfoVec.begin(),
                    numaClusterInfo[nid].occupiedInfoVec.end(), dockerInfos[dockerId].bindClusters.back()),
                    numaClusterInfo[nid].occupiedInfoVec.end());
                dockerInfos[dockerId].bindClusters.pop_back();
                continue;
            }
            minusCpuSet.push_back(systemClusterInfo[clusterId].occupiedCpuCores.back());
            systemClusterInfo[clusterId].freeCpuCores.push_back(systemClusterInfo[clusterId].occupiedCpuCores.back());
            systemClusterInfo[clusterId].occupiedCpuCores.pop_back();
            --needReturnCount;
        }
    }

    std::vector<int> tmp;
    for (int cpu : dockerInfos[dockerId].currentCpuSet) {
        if (std::find(minusCpuSet.begin(), minusCpuSet.end(), cpu) == minusCpuSet.end()) {
            tmp.push_back(cpu);
        }
    }
    dockerInfos[dockerId].currentCpuSet = tmp;

    std::string cpusetStr = ConvertCpusetToString(dockerInfos[dockerId].currentCpuSet);
    dockerInfos[dockerId].currentCpuLimit -= limit;
    std::string cpusStr = std::to_string(dockerInfos[dockerId].currentCpuLimit);
    std::string updateCpusCmd = "docker update --cpus=" + cpusStr + " " + dockerId;
    std::string updateCpuSetCmd = "docker update --cpuset-cpus=" + cpusetStr + " " + dockerId;
    std::string updateCpusResult = "";
    std::string updateCpuSetResult = "";
    if (ExecuteCMD(updateCpusCmd, updateCpusResult) < 0 ||
        ExecuteCMD(updateCpuSetCmd, updateCpuSetResult) < 0) {
        ERROR(logger, "Docker " << dockerId << " update cpuset-cpus and cpus failed.");
        return false;
    }
    INFO(logger, "Successfully update docker " << dockerId << " with CPUs: " << cpusetStr);
    INFO(logger, "docker " << dockerId << " minus " << limit << " cpus, current cpulimit is " << 
        dockerInfos[dockerId].currentCpuLimit);
    return true;
}

void ClusterAffinityAdapt::TryBorrowCpuFromIdleDocker()
{
    double couldBorrowLimit = 0.0;
    std::vector<std::string> couldLendDockers;
    for (auto &p : dockerInfos) {
        if (p.second.dockerLended || !p.second.borrowDockerMap.empty()) {
            // If docker has lent or has borrow, continue
            continue;
        }
        // When docker cpu usage below 50%, could lend cpu
        if ((p.second.cpuUsageQueue.size() == CPU_USAGE_QUEUE_LENGTH) && 
            (p.second.cpuUsageSum / CPU_USAGE_QUEUE_LENGTH <= p.second.initialCpuLimit * 100 * 0.5)) {
            p.second.lendCpuLimit = (p.second.initialCpuLimit) - (p.second.cpuUsageSum / CPU_USAGE_QUEUE_LENGTH / 100);
            // can't lend more than 50%
            p.second.lendCpuLimit = std::min(p.second.lendCpuLimit, p.second.initialCpuLimit * 0.5);
            couldLendDockers.push_back(p.first);
            couldBorrowLimit += p.second.lendCpuLimit;
        }
    }
    if (couldBorrowLimit < 0.01) {
        return;
    }
    std::vector<std::string> needBorrowDockers;
    for (auto &p : dockerInfos) {
        if (std::find(couldLendDockers.begin(), couldLendDockers.end(), p.first) != couldLendDockers.end()) {
            continue;
        }
        if ((p.second.cpuUsageSum / p.second.cpuUsageQueue.size()) >= p.second.initialCpuLimit * 100.0 * 0.90) {
            needBorrowDockers.push_back(p.first);
        }
    }

    if (needBorrowDockers.empty()) {
        return;
    }

    double borrowLimitPerDocker = couldBorrowLimit / needBorrowDockers.size();
    for (auto &dockerId : needBorrowDockers) {
        if (DockerAddCpuLimitComm(dockerId, borrowLimitPerDocker) == false) {
            ERROR(logger, "docker " << dockerId << "borrow cpu from idle docker failed.");
        }
        for (auto &lendDockerId : couldLendDockers) {
            dockerInfos[dockerId].borrowDockerMap[lendDockerId] = 
                dockerInfos[lendDockerId].lendCpuLimit / needBorrowDockers.size();
        }
    }
    for (auto dockerId : couldLendDockers) {
        if (DockerMinusCpuLimitComm(dockerId, dockerInfos[dockerId].lendCpuLimit) == false) {
            ERROR(logger, "docker " << dockerId << " lend cpu to busy docker failed.");
            continue;
        }
        dockerInfos[dockerId].dockerLended = true;
    }
    return;
}

void ClusterAffinityAdapt::TryReturnCpuToIdleDocker()
{
    std::vector<std::string> needReturnDocker;
    for (auto &p : dockerInfos) {
        if (p.second.dockerLended == false) {
            continue;
        }
        if((p.second.cpuUsageSum / p.second.cpuUsageQueue.size()) >=
            p.second.currentCpuLimit * 100.0 * 0.95) {
            needReturnDocker.push_back(p.first);
        }
    }
    if (needReturnDocker.empty()) {
        // no need return docker
        return;
    }
    std::unordered_map<std::string, double> returnDockerCpus;
    for (auto& needReturnDockerId : needReturnDocker) {
        for (auto &p : dockerInfos) {
            if (p.second.borrowDockerMap.count(needReturnDockerId) == 0) {
                continue;
            }
            // giva back borrowed docker cpu
            returnDockerCpus[p.first] += p.second.borrowDockerMap[needReturnDockerId];
            p.second.borrowDockerMap.erase(needReturnDockerId);
        }
    }
    for (auto& needReturnDockerId : needReturnDocker) {
        if (DockerAddCpuLimitComm(needReturnDockerId, dockerInfos[needReturnDockerId].lendCpuLimit) == false) {
            ERROR(logger, "docker " << needReturnDockerId << " borrow cpu from idle docker failed.");
        }
        dockerInfos[needReturnDockerId].dockerLended = false;
    }
    for (auto &p : returnDockerCpus) {
        if (DockerMinusCpuLimitComm(p.first, p.second) == false) {
            ERROR(logger, "docker " << p.first << " return cpu to idle docker failed.");
        }
    }
    return;
}

std::string ClusterAffinityAdapt::ConvertCpusetToString(std::vector<int>& nums)
{
    if (nums.empty()) {
        return "";
    }

    std::ostringstream result;

    int start = nums[0];
    int end = nums[0];

    for (size_t i = 1; i < nums.size(); ++i) {
        if (nums[i] == end + 1) {
            end = nums[i];
        } else {
            if (start == end) {
                result << start;
            } else {
                result << start << "-" << end;
            }
            result << ",";
            start = end = nums[i];
        }
    }

    if (start == end) {
        result << start;
    } else {
        result << start << "-" << end;
    }

    return result.str();
}

std::vector<int> ClusterAffinityAdapt::TransformStrToVec(const std::string& resourceListStr)
{
    std::stringstream ss(resourceListStr);
    std::string range;
    std::vector<int> resultSet;

    while (std::getline(ss, range, ',')) {
        size_t dashPos = range.find('-');
        if (dashPos == std::string::npos) {
            resultSet.push_back(std::stoi(range));
        } else {
            int start = std::stoi(range.substr(0, dashPos));
            int end = std::stoi(range.substr(dashPos + 1));
            for (int res = start; res <= end; ++res) {
                resultSet.push_back(res);
            }
        }
    }

    return resultSet;
}

int ClusterAffinityAdapt::SelectBindingNuma(const std::string &dockerId)
{
    if (dockerInfos[dockerId].currentMemSet.size() == 1) {
        return dockerInfos[dockerId].currentMemSet[0];
    }
    for (size_t i = 0; i < cpuTopology.size(); ++i) {
        if (dockerInfos[dockerId].currentCpuSet.front() >= cpuTopology[i].front() &&
            dockerInfos[dockerId].currentCpuSet.back() <= cpuTopology[i].back()) {
            return i;
        }
    }

    std::vector<int> used(cpuTopology.size(), 0);
    return clusterSelector.SelectIdleNode(used);
}

bool ClusterAffinityAdapt::DockerBindClusters(const std::string &dockerId, int nid, int clusterNum)
{
    std::vector<int> bindCpuSet;
    std::vector<int> bindMemSet = {nid};
    int clusterCpuNum = clusterTopology.GetOneClusterCpuNumber();
    if (clusterCpuNum == 0) {
        WARN(logger, "System don't support cluster architecture.");
        return false;
    }
    int clusterBeginIndex = cpuTopology[nid][0] / clusterCpuNum;
    int numaClusterNum = cpuTopology[nid].size() / clusterCpuNum;
    std::vector<int> used(numaClusterNum, 0);

    for (int clusterId : numaClusterInfo[nid].occupiedInfoVec) {
        int usedIndex = clusterId - clusterBeginIndex;
        used[usedIndex] = 1;
    }

    // select idle cluster by l3c_hit pmu counter
    std::vector<int> tmpBindClusters;
    for (int i = 0; i < clusterNum; ++i) {
        int clusterId = clusterSelector.SelectIdleCluster(nid, used);
        used[clusterId - clusterBeginIndex] = 1;
        numaClusterInfo[nid].occupiedInfoVec.push_back(clusterId);
        dockerInfos[dockerId].bindClusters.push_back(clusterId);
        tmpBindClusters.push_back(clusterId);
    }

    // refresh free cluster vec
    std::vector<int> tmpFreeInfoVec;
    for (int clusterId : numaClusterInfo[nid].freeInfoVec) {
        if (std::find(numaClusterInfo[nid].occupiedInfoVec.begin(),
                      numaClusterInfo[nid].occupiedInfoVec.end(),
                      clusterId) == numaClusterInfo[nid].occupiedInfoVec.end()) {
            tmpFreeInfoVec.push_back(clusterId);
        }
    }
    numaClusterInfo[nid].freeInfoVec = tmpFreeInfoVec;

    size_t needCpuCores = static_cast<size_t>(std::ceil(dockerInfos[dockerId].initialCpuLimit));
    for (size_t i = 0; i < tmpBindClusters.size() - 1; i++) {
        bindCpuSet.insert(bindCpuSet.end(),
            systemClusterInfo[tmpBindClusters[i]].freeCpuCores.begin(),
            systemClusterInfo[tmpBindClusters[i]].freeCpuCores.end());
        systemClusterInfo[tmpBindClusters[i]].occupiedCpuCores.insert(
            systemClusterInfo[tmpBindClusters[i]].occupiedCpuCores.end(),
            systemClusterInfo[tmpBindClusters[i]].freeCpuCores.begin(),
            systemClusterInfo[tmpBindClusters[i]].freeCpuCores.end());
        needCpuCores -= systemClusterInfo[tmpBindClusters[i]].freeCpuCores.size();
        systemClusterInfo[tmpBindClusters[i]].freeCpuCores.clear();
    }

    int clusterId = tmpBindClusters.back();
    std::vector<int> cpuUsed(clusterCpuNum, 1);
    int clusterCpuStart = clusterId * clusterCpuNum;
    for (int cpu : systemClusterInfo[clusterId].freeCpuCores) {
        cpuUsed[cpu - clusterCpuStart] = 0;
    }
    for (size_t i = 0; i < needCpuCores; ++i) {
        int cpu = clusterSelector.SelectIdleCpuFromCluster(clusterId, cpuUsed);
        cpuUsed[cpu - clusterCpuStart] = 1;
        systemClusterInfo[clusterId].occupiedCpuCores.push_back(cpu);
        bindCpuSet.push_back(cpu);
    }
    std::vector<int> tmp;
    for (int cpu : systemClusterInfo[clusterId].freeCpuCores) {
        if (std::find(bindCpuSet.begin(), bindCpuSet.end(), cpu) == bindCpuSet.end()) {
            tmp.push_back(cpu);
        }
    }
    systemClusterInfo[clusterId].freeCpuCores = tmp;

    return BindCpusAndMems(dockerId, bindCpuSet, bindMemSet);
}

bool ClusterAffinityAdapt::BindCpusAndMems(const std::string& dockerId,
                                      std::vector<int> cSet,
                                      std::vector<int> mSet)
{
    std::string cpusetStr = ConvertCpusetToString(cSet);
    std::string memorysetStr = ConvertCpusetToString(mSet);
    std::string updateCpuSetCmd = "docker update --cpuset-cpus=" + cpusetStr + " " + dockerId;
    std::string updateMemSetCmd = "docker update --cpuset-mems=" + memorysetStr + " " + dockerId;
    std::string updateCpuSetResult = "";
    std::string updateMemSetResult = "";
    if (ExecuteCMD(updateCpuSetCmd, updateCpuSetResult) < 0 ||
        ExecuteCMD(updateMemSetCmd, updateMemSetResult) < 0) {
        ERROR(logger, "Bind docker " << dockerId << " CpusAndMems failed.");
        return false;
    }

    INFO(logger, "Successfully bound docker " << dockerId
              << " to NUMA node " << memorysetStr
              << " with CPU: " << cpusetStr);
    
    dockerInfos[dockerId].currentCpuSet = cSet;
    dockerInfos[dockerId].currentMemSet = mSet;

    return true;
}

void ClusterAffinityAdapt::DockerOptimizeBindSchedule()
{
    for (auto &p : dockerInfos) {
        if (p.second.optimizeBinded == true) {
            continue;
        }

        if (p.second.initialCpuLimit <= 0) {
            ERROR(logger, p.first << " CpuLimit=" << p.second.initialCpuLimit);
            continue;
        }

        size_t clusterCpuNum = clusterTopology.GetOneClusterCpuNumber();
        if (clusterCpuNum == 0) {
            WARN(logger, "System don't support cluster architecture.");
            return;
        }
        if (oeaware::IsSmtEnable()) {
            clusterCpuNum /= 2;
        }
        size_t needClusterNum = static_cast<size_t>(std::ceil(p.second.initialCpuLimit / clusterCpuNum));
        int bindNuma = SelectBindingNuma(p.first);
        if (numaClusterInfo[bindNuma].freeInfoVec.size() < needClusterNum) {
            ERROR(logger, "NUMA" << bindNuma << " have " << numaClusterInfo[bindNuma].freeInfoVec.size() <<
                " clusters left, docker " << p.first << " need " << needClusterNum << " clusters.");
            return;
        }
        DockerBindClusters(p.first, bindNuma, needClusterNum);

        p.second.optimizeBinded = true;
    }
}

#define CMD_RESULT_BUF_SIZE 512
int ClusterAffinityAdapt::ExecuteCMD(const std::string &cmd, std::string &result)
{
    std::string ps = cmd;
    FILE *ptr = nullptr;

    if (ps.size() > CMD_RESULT_BUF_SIZE || ps.empty()) {
        ERROR(logger, "cmd max len " << CMD_RESULT_BUF_SIZE);
        return -1;
    }

    if((ptr = popen(ps.c_str(), "r")) == nullptr) {
        ERROR(logger, "popen error " << ps);
        return -1;
    }

    char *bufPs = new char[CMD_RESULT_BUF_SIZE];
    while(fgets(bufPs, CMD_RESULT_BUF_SIZE, ptr) != nullptr) {
        result += bufPs;
    }
    pclose(ptr);
    ptr = nullptr;
    delete[] bufPs;
    
    return 0;
}
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
#include "load_based_scheduling.h"
#include <yaml-cpp/yaml.h>
#include <oeaware/data/env_data.h>
#include <oeaware/data/pmu_sampling_data.h>
#include "oeaware/utils.h"

namespace oeaware {
LoadBasedScheduling::LoadBasedScheduling()
{
    name = OE_LOAD_BASED_SCHEDULING_TUNE;
    description = "";
    version = "1.0.0";
    period = PERIOD;
    priority = PRIORITY;
    type = oeaware::TUNE;
    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = this->name;
    supportTopics.push_back(topic);
    subscribeTopics.emplace_back(Topic{OE_DOCKER_COLLECTOR, OE_DOCKER_COLLECTOR, ""});
    subscribeTopics.emplace_back(Topic{OE_ENV_INFO, "static", ""});
    subscribeTopics.emplace_back(Topic{OE_PMU_SAMPLING_COLLECTOR, "cycles", ""});
}

Result LoadBasedScheduling::OpenTopic(const oeaware::Topic &topic)
{
    return Result(OK);
}

void LoadBasedScheduling::CloseTopic(const oeaware::Topic &topic)
{
}

static int ReadSysParam(const std::string &path, std::string &value)
{
    std::ifstream file(path);
    std::string line;

    if (!file.is_open()) {
        return -1;
    }
    file >> value;
    file.close();
    return 0;
}

void LoadBasedScheduling::UpdateDockerData(const DataList &dataList)
{
    std::unordered_set<std::string> containerIds;
    for (size_t i = 0; i < dataList.len; ++i) {
        auto container = static_cast<Container*>(dataList.data[i]);
        int tot = 0;
        if (container->cfs_quota_us == unlimit) {
            tot = unlimit;
        } else {
            tot = container->cfs_quota_us / container->cfs_period_us;
        }
        if (!containers.count(container->id)) {
            std::string preferredCpu = "";
            auto path =  "/sys/fs/cgroup/cpuset/docker/" + container->id + "/cpuset.preferred_cpus";
            auto ret = ReadSysParam(path, preferredCpu);
            if (ret < 0) {
                WARN(logger, "read " << preferredCpu << " failed.");
            }
            containers[container->id].originPreferredCpu = preferredCpu;  
        }
        containers[container->id].id = container->id;
        containers[container->id].cpus = container->cpus;
        containers[container->id].totLoad = tot;
        containerIds.insert(container->id);
        for (auto pid : container->tasks) {
            pids[pid] = container->id; // 记录pid对应的容器id
            containers[container->id].tasks.insert(pid);
        }
    }
    for (auto it = containers.begin(); it != containers.end();) {
        if (containerIds.count(it->first) == 0) {
            // 如果容器不在当前数据中，则删除
            for (auto pid : it->second.tasks) {
                pids.erase(pid); // 删除pid对应的容器id
            }
            it = containers.erase(it);
        } else {
            ++it;
        }
    }
}

void LoadBasedScheduling::UpdateNumaInfo(const DataList &dataList)
{
    if (!cpuNuma.empty()) {
        return;
    }
    for (size_t i = 0; i < dataList.len; ++i) {
        auto envStaticInfo = static_cast<EnvStaticInfo*>(dataList.data[i]);
        for (int cpu = 0; cpu < envStaticInfo->cpuNumConfig; ++cpu) {
            int node = envStaticInfo->cpu2Node[cpu];
            cpuNuma[cpu] = node;
            numaContainers[node].cpus.emplace_back(cpu);
        }
    }
}

void LoadBasedScheduling::InitCpuCycles(int cpuNum)
{
    maxCycles.resize(cpuNum, 0);
    for (int cpu = 0; cpu < cpuNum; cpu++) {
        maxCycles[cpu] = GetCpuCycles(cpu);
        if (maxCycles[cpu] == 0) {
            maxCycles[cpu] = GetCpuFreqByDmi();
        }
    }
}

void LoadBasedScheduling::UpdatePmuData(const DataList &dataList)
{
    auto pmuData = static_cast<PmuSamplingData*>(dataList.data[0]);
    for (int i = 0; i < pmuData->len; ++i) {
        auto &data = pmuData->pmuData[i];
        if (!pids.count(data.pid)) {
            continue; // 只处理当前容器的pid
        }
        containers[pids[data.pid]].cycles += data.period;
    }
    static int ms = 1000;
    for (auto &item : containers) {
        auto &container = item.second;
        container.cpuLoad = container.cycles * 1.0 / pmuData->interval / maxCycles[0] * ms;
        container.cycles = 0; // 重置cycles
    }
}

void LoadBasedScheduling::UpdateData(const DataList &dataList)
{
    Topic topic{dataList.topic.instanceName, dataList.topic.topicName, dataList.topic.params};
    if (topic.instanceName == OE_DOCKER_COLLECTOR && topic.topicName == OE_DOCKER_COLLECTOR) {
        UpdateDockerData(dataList);
    } else if (topic.instanceName == OE_ENV_INFO && topic.topicName == "static") {
        UpdateNumaInfo(dataList);
    } else if (topic.instanceName == OE_PMU_SAMPLING_COLLECTOR && topic.topicName == "cycles") {
        // 处理PMU采样数据
        UpdatePmuData(dataList);
    } else {
        WARN(logger, "Unknown topic, {instanceName:" + topic.instanceName + ", topicName:" + topic.topicName + "}.");
    }
}

Result LoadBasedScheduling::ReadConfig(const std::string &path)
{
    if (!FileExist(path)) {
        return Result(FAILED, "config file does not exist, {path:" + path + "}.");
    }
    try {
        YAML::Node node = YAML::LoadFile(path);
        if (node["high_load"]) {
            double highLoad = node["high_load"].as<double>();
            highLoadThreshold = highLoad;
        }
        if (highLoadThreshold < 0 || highLoadThreshold > 1) {
            return Result(FAILED, "config error, high_load must be in [0,1].");
        }
    } catch (const YAML::Exception &e) {
        return Result(FAILED, e.what());
    }
    return Result(OK);
}

Result LoadBasedScheduling::Enable(const std::string &param)
{
    (void)param;
    auto ret = ReadConfig(configPath);
    if (ret.code != OK) {
        return ret;
    }
    for (auto &topic : subscribeTopics) {
        Result ret = Subscribe(topic);
        if (ret.code != OK) {
            return ret;
        }
    }
    int cpuNum = sysconf(_SC_NPROCESSORS_CONF);
    InitCpuCycles(cpuNum);
    return Result(OK);
}

static int WriteSysParam(const std::string &path, const std::string &value)
{
    std::ofstream sysFile(path);
    if (!sysFile.is_open()) {
        return -1;
    }
    sysFile << value;
    if (sysFile.fail()) {
        return -1;
    }
    sysFile.close();
     if (sysFile.fail()) {
        return -1;
    }
    return 0;
}

void LoadBasedScheduling::Disable()
{
    tuneContainers.clear();
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    for (auto &item : containers) {
        // 清除容器的cpuset.preferred_cpus
        std::string s = "echo " + item.second.originPreferredCpu + " > /sys/fs/cgroup/cpuset/docker/" + item.first + "/cpuset.preferred_cpus";
        system(s.c_str());
        
    }
    containers.clear();
    usedCpus.clear();
    cpuNuma.clear();
    containerNuma.clear();
    numaContainers.clear();
    pids.clear();
}

bool LoadBasedScheduling::IsHighLoad(const ContainerCpuInfo &container, int cpuNum)
{
    // 判断容器是否负载过高
    if (container.totLoad == unlimit) {
        return container.cpuLoad > highLoadThreshold * cpuNum;
    }
    return container.cpuLoad > highLoadThreshold * container.totLoad;
}

void LoadBasedScheduling::Run()
{
    std::vector<std::string> tuneContainerIds;
    for (auto &item : numaContainers) {
        item.second.containerNum = 0; // 重置numa节点的容器数量
        item.second.usedIndex = 0; // 重置numa节点的已使用cpu索引
    }
    for (const auto &item : containers) {
        auto container = item.second;
        auto cpus = ParseRange(container.cpus);
        if (!IsHighLoad(container, cpus.size())) {
            continue;
        }
        // 获取numa节点的上容器的个数
        numaContainers[cpuNuma[cpus[0]]].containerNum++;
        // 记录容器对应的numa节点
        containerNuma[container.id] = cpuNuma[cpus[0]];
        tuneContainerIds.emplace_back(container.id);
    }
    
    for (const auto &containerId : tuneContainerIds) {
        int node = containerNuma[containerId];
        // 每个容器分配的cpu数量
        int num =  numaContainers[node].cpus.size() / numaContainers[node].containerNum;
        std::string cpusStr;
        for (int i = 0; i < num; ++i) {
            int cpuIndex = numaContainers[node].usedIndex + i;
            if (tuneContainerIds.size() == 1 && usedCpus.count(numaContainers[node].cpus[cpuIndex])) {
                continue;
            }
            cpusStr += std::to_string(numaContainers[node].cpus[numaContainers[node].usedIndex + i]);
            cpusStr += ",";
        }
        if (WriteSysParam("/sys/fs/cgroup/cpuset/docker/" + containerId + "/cpuset.preferred_cpus", cpusStr) < 0) {
            WARN(logger, "Write cpuset.preferred_cpus failed, {containerId:" + containerId + ", cpus:" + cpusStr +
                "}.");
            continue;
        }
        INFO(logger, "Write cpuset.preferred_cpus success, {containerId:" + containerId + ", cpus:" + cpusStr + "}.");
        numaContainers[node].usedIndex += num;
    }
}
}

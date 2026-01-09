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
#include "soft_domain_tune.h"
#include "oeaware/utils.h"
#include <fstream>
#include <sstream>
#include <climits>
#include <thread>
#include <chrono>
#include <numa.h>
#include <unistd.h>

using namespace oeaware;

SoftDomainTune::SoftDomainTune()
{
    name = "soft_domain_tune";
    description = "Enable SOFT_DOMAIN scheduling feature";
    version = "1.0.0";
    period = defaultPeriod;
    priority = defaultPriority;
    type = TUNE;
    
    // 订阅docker采集数据
    Topic dockerTopic;
    dockerTopic.instanceName = OE_DOCKER_COLLECTOR;
    dockerTopic.topicName = OE_DOCKER_COLLECTOR;
    subscribeTopics.push_back(dockerTopic);
    
    // 订阅线程采集数据
    Topic threadTopic;
    threadTopic.instanceName = OE_THREAD_COLLECTOR;
    threadTopic.topicName = OE_THREAD_COLLECTOR;
    subscribeTopics.push_back(threadTopic);
    
    // 订阅环境信息数据（用于获取NUMA和CPU信息）
    Topic envTopic;
    envTopic.instanceName = OE_ENV_INFO;
    envTopic.topicName = "static";
    subscribeTopics.push_back(envTopic);
}

oeaware::Result SoftDomainTune::OpenTopic(const Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void SoftDomainTune::CloseTopic(const Topic &topic)
{
    (void)topic;
}

void SoftDomainTune::UpdateData(const DataList &dataList)
{
    Topic topic{dataList.topic.instanceName, dataList.topic.topicName, dataList.topic.params};
    if (topic.instanceName == OE_DOCKER_COLLECTOR && topic.topicName == OE_DOCKER_COLLECTOR) {
        UpdateDockerData(dataList);
    } else if (topic.instanceName == OE_THREAD_COLLECTOR && topic.topicName == OE_THREAD_COLLECTOR) {
        UpdateThreadData(dataList);
    } else if (topic.instanceName == OE_ENV_INFO && topic.topicName == "static") {
        UpdateEnvData(dataList);
    } else {
        WARN(logger, "Unknown topic, {instanceName:" + topic.instanceName + ", topicName:" + topic.topicName + "}.");
    }
}

oeaware::Result SoftDomainTune::Enable(const std::string &param)
{
    (void)param;
    
    // 订阅所有topic（包括env_info，用于获取NUMA信息）
    for (auto &topic : subscribeTopics) {
        Result ret = Subscribe(topic);
        if (ret.code != OK) {
            ERROR(logger, "Failed to subscribe topic: " << topic.instanceName << "/" << topic.topicName);
            return ret;
        }
    }
    
    // 如果env数据还没准备好，从系统直接获取NUMA和CPU信息
    if (!envDataReady) {
        if (!InitEnvInfoFromSystem()) {
            ERROR(logger, "Failed to get NUMA and CPU info from system");
            return oeaware::Result(FAILED, "Failed to get system info");
        }
        DEBUG(logger, "Initialized env info from system: numaNum=" << numaNum << ", cpuNumConfig=" << cpuNumConfig);
    }
    
    // 加载并解析配置文件
    if (!LoadConfig()) {
        WARN(logger, "Failed to load config file, using default configuration");
        // 默认配置：什么也不配置
    } else {
        // 校验配置
        if (!ValidateConfig()) {
            return oeaware::Result(FAILED, "Config validation failed: cpu_num exceeds single NUMA CPU count");
        }
    }
    
    // TODO: 实现具体功能
    return oeaware::Result(OK, "Soft domain tune enabled");
}

void SoftDomainTune::Disable()
{
    // 取消订阅
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    
    // 清空docker信息
    dockerContainers.clear();
    
    // 清空线程信息
    threadInfos.clear();
    
    // 清空配置
    configItems.clear();
    
    // 清空环境信息
    envDataReady = false;
    numaNum = 0;
    cpuNumConfig = 0;
    cpu2Node.clear();
    numaCpuCounts.clear();
    
    // TODO: 实现具体功能
}

void SoftDomainTune::Run()
{
    // 每个周期都会调用Run()，此时dockerContainers和threadInfos已经通过UpdateData更新
    // TODO: 实现具体功能，可以使用dockerContainers和threadInfos中的数据
}

void SoftDomainTune::UpdateDockerData(const DataList &dataList)
{
    // 使用unordered_set记录当前周期收到的docker ID
    std::unordered_set<std::string> currentDockerIds;
    
    // 更新docker信息
    for (uint64_t i = 0; i < dataList.len; i++) {
        auto *container = static_cast<Container*>(dataList.data[i]);
        if (container == nullptr) {
            continue;
        }
        
        // 存储或更新docker信息
        dockerContainers[container->id] = *container;
        currentDockerIds.insert(container->id);
    }
    
    // 移除已经不存在的docker（如果某个docker在当前数据中不存在，说明它已经被删除）
    for (auto it = dockerContainers.begin(); it != dockerContainers.end();) {
        if (currentDockerIds.find(it->first) == currentDockerIds.end()) {
            // docker已不存在，从map中移除
            it = dockerContainers.erase(it);
        } else {
            ++it;
        }
    }
    
    DEBUG(logger, "Updated docker data, current docker count: " << dockerContainers.size());
}

void SoftDomainTune::UpdateThreadData(const DataList &dataList)
{
    // 使用unordered_set记录当前周期收到的线程ID
    std::unordered_set<int> currentThreadIds;
    
    // 更新线程信息
    for (uint64_t i = 0; i < dataList.len; i++) {
        auto *thread = static_cast<ThreadInfo*>(dataList.data[i]);
        if (thread == nullptr) {
            continue;
        }
        
        // 存储或更新线程信息（tid -> name）
        if (thread->name != nullptr) {
            threadInfos[thread->tid] = std::string(thread->name);
        } else {
            threadInfos[thread->tid] = "";
        }
        currentThreadIds.insert(thread->tid);
    }
    
    // 移除已经不存在的线程（如果某个线程在当前数据中不存在，说明它已经被删除）
    for (auto it = threadInfos.begin(); it != threadInfos.end();) {
        if (currentThreadIds.find(it->first) == currentThreadIds.end()) {
            // 线程已不存在，从map中移除
            it = threadInfos.erase(it);
        } else {
            ++it;
        }
    }
    
    DEBUG(logger, "Updated thread data, current thread count: " << threadInfos.size());
}

void SoftDomainTune::UpdateEnvData(const DataList &dataList)
{
    if (dataList.len == 0 || dataList.data[0] == nullptr) {
        return;
    }
    
    auto *envStaticInfo = static_cast<EnvStaticInfo*>(dataList.data[0]);
    if (envStaticInfo == nullptr) {
        return;
    }
    
    // 只更新一次，因为static信息不会变
    if (envDataReady) {
        return;
    }
    
    // 更新环境信息
    numaNum = envStaticInfo->numaNum;
    cpuNumConfig = envStaticInfo->cpuNumConfig;
    
    // 更新CPU到NUMA节点的映射
    cpu2Node.clear();
    cpu2Node.resize(cpuNumConfig);
    for (int i = 0; i < cpuNumConfig; i++) {
        cpu2Node[i] = envStaticInfo->cpu2Node[i];
    }
    
    // 计算每个NUMA节点的CPU数量
    numaCpuCounts.clear();
    numaCpuCounts.resize(numaNum, 0);
    for (int i = 0; i < cpuNumConfig; i++) {
        if (cpu2Node[i] >= 0 && cpu2Node[i] < numaNum) {
            numaCpuCounts[cpu2Node[i]]++;
        }
    }
    
    envDataReady = true;
    DEBUG(logger, "Updated env data from subscription: numaNum=" << numaNum << ", cpuNumConfig=" << cpuNumConfig);
}

bool SoftDomainTune::LoadConfig()
{
    if (!FileExist(configPath)) {
        return false;
    }
    
    try {
        YAML::Node config = YAML::LoadFile(configPath);
        return ParseConfig(config);
    } catch (const std::exception &e) {
        ERROR(logger, "Failed to load config: " << e.what());
        return false;
    }
}

bool SoftDomainTune::ParseConfig(const YAML::Node &node)
{
    if (!node.IsSequence()) {
        ERROR(logger, "Config file format error: root node must be a sequence");
        return false;
    }
    
    configItems.clear();
    for (const auto &item : node) {
        SoftDomainConfigItem configItem;
        
        // 解析type
        if (!item["type"]) {
            ERROR(logger, "Config item missing 'type' field");
            continue;
        }
        std::string typeStr = item["type"].as<std::string>();
        if (typeStr == "docker") {
            configItem.type = SoftDomainConfigType::DOCKER;
        } else if (typeStr == "process") {
            configItem.type = SoftDomainConfigType::PROCESS;
        } else {
            ERROR(logger, "Invalid config type: " << typeStr << ", must be 'docker' or 'process'");
            continue;
        }
        
        // 解析whitelist
        if (item["whitelist"] && item["whitelist"].IsSequence()) {
            for (const auto &wl : item["whitelist"]) {
                configItem.whitelist.push_back(wl.as<std::string>());
            }
        }
        
        // 解析cpu_num
        if (item["cpu_num"]) {
            configItem.cpuNum = item["cpu_num"].as<std::string>();
        }
        
        configItems.push_back(configItem);
    }
    
    return true;
}

bool SoftDomainTune::InitEnvInfoFromSystem()
{
    // 从系统直接获取NUMA节点数量
    numaNum = numa_num_configured_nodes();
    if (numaNum <= 0) {
        ERROR(logger, "Failed to get NUMA node count from system");
        return false;
    }
    
    // 从系统直接获取CPU配置数量
    cpuNumConfig = sysconf(_SC_NPROCESSORS_CONF);
    if (cpuNumConfig <= 0) {
        ERROR(logger, "Failed to get CPU count from system");
        return false;
    }
    
    // 通过 cpuNumConfig / numaNum 计算单个NUMA的CPU数量（简单估算）
    // 注意：这是估算值，实际每个NUMA的CPU数量可能不同
    // 后续UpdateEnvData会更新准确的numaCpuCounts
    int estimatedCpuPerNuma = (numaNum != 0) ? (cpuNumConfig / numaNum) : 0;
    numaCpuCounts.clear();
    numaCpuCounts.resize(numaNum, estimatedCpuPerNuma);
    
    envDataReady = true;
    DEBUG(logger, "Initialized env info from system: numaNum=" << numaNum 
          << ", cpuNumConfig=" << cpuNumConfig 
          << ", estimatedCpuPerNuma=" << estimatedCpuPerNuma);
    
    return true;
}

int SoftDomainTune::GetCpuCountPerNuma()
{
    if (!envDataReady || numaNum <= 0) {
        ERROR(logger, "Env data not ready, cannot get CPU count per NUMA");
        return 0;
    }
    
    // 如果numaCpuCounts已经计算好（从订阅数据），返回最小的CPU数量
    if (!numaCpuCounts.empty()) {
        int minCpuCount = INT_MAX;
        for (int count : numaCpuCounts) {
            if (count > 0 && count < minCpuCount) {
                minCpuCount = count;
            }
        }
        if (minCpuCount != INT_MAX) {
            return minCpuCount;
        }
    }
    
    // 否则通过 cpuNumConfig / numaNum 计算（简单估算）
    if (cpuNumConfig > 0 && numaNum > 0) {
        return cpuNumConfig / numaNum;
    }
    
    return 0;
}

bool SoftDomainTune::ValidateConfig()
{
    int maxCpuPerNuma = GetCpuCountPerNuma();
    if (maxCpuPerNuma <= 0) {
        ERROR(logger, "Failed to get CPU count per NUMA node");
        return false;
    }
    
    for (const auto &item : configItems) {
        if (item.cpuNum.empty()) {
            // cpu_num为空，表示不配置，跳过校验
            continue;
        }
        
        try {
            int cpuNum = std::stoi(item.cpuNum);
            if (cpuNum <= 0) {
                ERROR(logger, "Invalid cpu_num: " << item.cpuNum << ", must be positive");
                return false;
            }
            
            if (cpuNum > maxCpuPerNuma) {
                ERROR(logger, "cpu_num (" << cpuNum << ") exceeds single NUMA CPU count (" 
                      << maxCpuPerNuma << ")");
                return false;
            }
        } catch (const std::exception &e) {
            ERROR(logger, "Invalid cpu_num format: " << item.cpuNum << ", error: " << e.what());
            return false;
        }
    }
    
    return true;
}


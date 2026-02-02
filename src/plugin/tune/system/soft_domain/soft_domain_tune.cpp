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
#include <regex>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <iostream>
#include <cerrno>
#include <cstring>

using namespace oeaware;

SoftDomainTune::SoftDomainTune()
{
    name = "soft_domain_tune";
    description = "Enable SOFT_DOMAIN scheduling feature";
    version = "1.0.0";
    period = defaultPeriod;
    priority = defaultPriority;
    type = TUNE;
    
    subscribeTopics.push_back(oeaware::Topic{OE_DOCKER_COLLECTOR, OE_DOCKER_COLLECTOR, ""});
    subscribeTopics.push_back(oeaware::Topic{OE_THREAD_COLLECTOR, OE_THREAD_COLLECTOR, ""});
    subscribeTopics.push_back(oeaware::Topic{OE_ENV_INFO, "static", ""});
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
    // env数据还没准备好，从系统直接获取NUMA和CPU信息
    if (!InitEnvInfoFromSystem()) {
        ERROR(logger, "Failed to get NUMA and CPU info from system");
        return oeaware::Result(FAILED, "Failed to get system info");
    }
    INFO(logger, "Initialized env info from system: numaNum=" << numaNum << ", cpuNumConfig=" << cpuNumConfig);
    
    
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
    
    // 初始化缓存的配置项（避免每次Run时重新收集）
    InitConfigsCache();
    
    // 使用ReadSchedFeatures函数初始化sched_features路径并检查SOFT_DOMAIN是否支持
    std::vector<std::string> features;
    if (!ReadSchedFeatures(schedFeaturesPath, features)) {
        return oeaware::Result(FAILED, "Failed to find sched features file");
    }
    
    // 检查SOFT_DOMAIN是否支持
    bool softDomainSupported = false;
    for (const auto &feature : features) {
        if (feature.find("SOFT_DOMAIN") != std::string::npos) {
            softDomainSupported = true;
            break;
        }
    }
    
    if (!softDomainSupported) {
        return oeaware::Result(FAILED, "SOFT_DOMAIN feature is not supported in kernel");
    }
    
    // 启动SOFT_DOMAIN调度特性
    if (!WriteValueToFile(schedFeaturesPath, "SOFT_DOMAIN")) {
        return oeaware::Result(FAILED, "Failed to enable SOFT_DOMAIN feature");
    }
    
    // 初始化NUMA剩余配额（初始为每个NUMA的CPU数量）
    numaRemainingQuota.clear();
    numaRemainingQuota.resize(numaNum, cpuCntPerNuma);

    for (auto &topic : subscribeTopics) {
        Result ret = Subscribe(topic);
        if (ret.code != OK) {
            ERROR(logger, "Failed to subscribe topic: " << topic.instanceName << "/" << topic.topicName);
            return ret;
        }
    }
    return oeaware::Result(OK, "Soft domain tune enabled");
}

void SoftDomainTune::Disable()
{
    // 先关闭所有参与优化的docker的soft_domain
    for (auto &pair : dockerInfos) {
        if (!pair.second.IsOptimized()) {
            continue;
        }
        const std::string &containerId = pair.first;
        std::string cgroupPath = "/sys/fs/cgroup/cpu/docker/" + containerId;
        
        // 关闭cpu.soft_domain（设置为0）
        if (WriteValueToFile(cgroupPath + "/cpu.soft_domain", "0")) { 
            INFO(logger, "Disabled soft_domain for docker: " << containerId);
        } else {
            WARN(logger, "Failed to disable soft_domain for docker: " << containerId);
        }
        
        // 关闭cpu.soft_domain_nr_cpu（设置为0）
        if (WriteValueToFile(cgroupPath + "/cpu.soft_domain_nr_cpu", "0")) {
            INFO(logger, "Disabled soft_domain_nr_cpu for docker: " << containerId);
        } else {
            WARN(logger, "Failed to disable soft_domain_nr_cpu for docker: " << containerId);
        }
    }
    
    // 清理进程cgroup（将进程移到根cgroup并删除cgroup目录）
    CleanupProcessCgroups();
    
    // 关闭总开关（回退SOFT_DOMAIN调度特性）
    if (!WriteValueToFile(schedFeaturesPath, "NO_SOFT_DOMAIN")) {
        ERROR(logger, "Failed to disable SOFT_DOMAIN feature");
    }
    
    // 取消订阅
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    
    // 清空docker信息
    dockerInfos.clear();
    
    // 清空线程信息
    processInfos.clear();
    
    // 清空配置
    configItems.clear();
    configsCache.clear();
    
    // 清空环境信息
    envDataReady = false;
    numaNum = 0;
    cpuNumConfig = 0;
    cpu2Node.clear();
    
    // 清理docker绑定信息
    numaRemainingQuota.clear();
    
    // 清理进程cgroup信息
    processCgroupsByCpuNum.clear();
    processPidToCgroupInfo.clear();
    processNameToCgroupCountByCpuNum.clear();
    
    INFO(logger, "Soft domain tune disabled");
}

void SoftDomainTune::Run()
{ 
    // 先对docker进行配置
    ConfigureDockerSoftDomain();
    
    // 然后对非docker的进程进行配置
    ConfigureProcessSoftDomain();

    // 打印各NUMA剩余配额
    std::string numaRemainingQuotaStr = "";
    for (int i = 0; i < numaNum; i++) {
        numaRemainingQuotaStr += "[" + std::to_string(i) + ", " + std::to_string(numaRemainingQuota[i]) + "]";
    }
    INFO(logger, "NUMA remaining quota: " << numaRemainingQuotaStr);
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
        
        // 存储或更新docker信息到dockerInfos
        auto &info = dockerInfos[container->id];
        info.id = container->id;
        info.container = *container;
        if (info.name.empty()) {
            // 首次创建时获取名称
            info.name = GetContainerName(container->id);
        }

        info.boundNumaNode = GetContainerNumaNode(*container);
        if(info.name.empty() || !envDataReady) {
            info.dataReady = false;
        } else {
            info.dataReady = true;
        }
        currentDockerIds.insert(container->id);
    }
    
    // 移除已删除的docker并恢复NUMA配额
    for (auto it = dockerInfos.begin(); it != dockerInfos.end();) {
        if (currentDockerIds.find(it->first) == currentDockerIds.end()) {
            const std::string &containerId = it->first;
            DockerInfo &info = it->second;
            
            // 如果容器已优化，恢复NUMA配额
            if (info.IsOptimized()) {
                numaRemainingQuota[info.softDomainNuma - 1] += info.softDomainNrCpu;
                INFO(logger, "Container " << containerId << " deleted, restored "
                    << info.softDomainNrCpu << " CPU quota to NUMA " << (info.softDomainNuma - 1));
            }
            
            it = dockerInfos.erase(it);
        } else {
            ++it;
        }
    }
    
    DEBUG(logger, "Updated docker data, current docker count: " << dockerInfos.size());
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
        
        // 存储或更新线程信息（pid -> name）
        if (thread->name != nullptr) {
            processInfos[thread->pid] = std::string(thread->name);
        } else {
            processInfos[thread->pid] = "";
        }
        currentThreadIds.insert(thread->pid);
    }
    
    // 移除已经不存在的线程（如果某个线程在当前数据中不存在，说明它已经被删除）
    for (auto it = processInfos.begin(); it != processInfos.end();) {
        if (currentThreadIds.find(it->first) == currentThreadIds.end()) {
            // 线程已不存在，从map中移除，同时更新cgroup分配信息
            int pid = it->first;
            const std::string &processName = it->second;
            
            // 如果该进程在cgroup中，更新计数
            auto cgroupIt = processPidToCgroupInfo.find(pid);
            if (cgroupIt != processPidToCgroupInfo.end()) {
                int cpuNum = cgroupIt->second.first;
                int cgroupIdx = cgroupIt->second.second;
                auto countMapIt = processNameToCgroupCountByCpuNum.find(cpuNum);
                if (countMapIt != processNameToCgroupCountByCpuNum.end()) {
                    auto countIt = countMapIt->second.find(processName);
                    if (countIt != countMapIt->second.end() && 
                        cgroupIdx >= 0 && cgroupIdx < static_cast<int>(countIt->second.size())) {
                        countIt->second[cgroupIdx]--;
                    }
                }
                processPidToCgroupInfo.erase(cgroupIt);
            }
            
            it = processInfos.erase(it);
        } else {
            ++it;
        }
    }
    
    DEBUG(logger, "Updated thread data, current thread count: " << processInfos.size());
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
            try {
                configItem.cpuNum = item["cpu_num"].as<int>();
            } catch (const std::exception &e) {
                ERROR(logger, "Failed to parse cpu_num as integer: " << e.what());
                configItem.cpuNum = 0;  // 解析失败，设置为0（未配置）
            }
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
    // 注意：这是估算值，实际每个NUMA的CPU数量可能不同, 目前仅支持各NUMA个数相同的机器，如果后续有需要再适配
    cpuCntPerNuma = (numaNum != 0) ? (cpuNumConfig / numaNum) : 0;
    if (cpuCntPerNuma <= 0) {
        ERROR(logger, "Failed to calculate cpuCntPerNuma");
        return false;
    }
    INFO(logger, "Initialized env info from system: numaNum=" << numaNum 
          << ", cpuNumConfig=" << cpuNumConfig 
          << ", cpuCntPerNuma=" << cpuCntPerNuma);
    
    return true;
}

bool SoftDomainTune::ValidateConfig()
{
    if (cpuCntPerNuma <= 0) {
        ERROR(logger, "cpuCntPerNuma is not initialized");
        return false;
    }
    
    for (const auto &item : configItems) {
        if (item.cpuNum <= 0) {
            // cpu_num为0或负数，表示未配置，跳过校验
            continue;
        }
        
        if (item.cpuNum > cpuCntPerNuma) {
            ERROR(logger, "cpu_num (" << item.cpuNum << ") exceeds single NUMA CPU count (" 
                  << cpuCntPerNuma << ")");
            return false;
        }
    }
    
    return true;
}

bool SoftDomainTune::CreateCgroupDir(const std::string &path)
{
    // 检查目录是否已存在
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return true;  // 目录已存在
    }
    
    // 创建目录
    if (mkdir(path.c_str(), 0755) != 0) {
        ERROR(logger, "Failed to create cgroup directory: " << path);
        return false;
    }
    
    INFO(logger, "Created cgroup directory: " << path);
    return true;
}

std::string SoftDomainTune::GetContainerName(const std::string &containerId)
{
    // 先检查dockerInfos
    auto it = dockerInfos.find(containerId);
    if (it != dockerInfos.end() && !it->second.name.empty()) {
        return it->second.name;
    }
    
    // 通过docker inspect命令获取容器名称
    std::string command = "docker inspect --format='{{.Name}}' " + containerId + " 2>/dev/null";
    std::string result;
    
    if (!ExecCommand(command, result)) {
        WARN(logger, "Failed to get container name for ID: " << containerId);
        // 缓存空字符串到dockerInfos，避免重复执行失败的命令
        dockerInfos[containerId].name = "";
        return "";
    }
    
    // 去除结果中的换行符和首尾空白
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    // 去除开头的斜杠（docker inspect返回的名称通常以/开头）
    if (!result.empty() && result[0] == '/') {
        result = result.substr(1);
    }
    
    // 更新dockerInfos
    dockerInfos[containerId].name = result;
    
    return result;
}

bool SoftDomainTune::MatchContainerName(const std::string &containerId, const std::vector<std::string> &whitelist)
{
    if (whitelist.empty()) {
        return false;
    }
    
    // 获取容器名称（通过docker inspect命令）
    std::string containerName = GetContainerName(containerId);
    if (containerName.empty()) {
        // 如果获取名称失败，返回false
        return false;
    }
    
    for (const auto &pattern : whitelist) {
        // 简单的通配符匹配：将*转换为.*
        std::string regexPattern = pattern;
        std::replace(regexPattern.begin(), regexPattern.end(), '*', '.');
        regexPattern = ".*" + regexPattern + ".*";
        
        try {
            std::regex re(regexPattern, std::regex_constants::icase);
            if (std::regex_search(containerName, re)) {
                return true;
            }
        } catch (const std::exception &e) {
            WARN(logger, "Regex match error: " << e.what());
        }
    }
    
    return false;
}

bool SoftDomainTune::MatchProcessName(const std::string &processName, const std::vector<std::string> &whitelist)
{
    if (whitelist.empty()) {
        return false;
    }
    
    for (const auto &pattern : whitelist) {
        // 简单的通配符匹配：将*转换为.*
        std::string regexPattern = pattern;
        std::replace(regexPattern.begin(), regexPattern.end(), '*', '.');
        regexPattern = ".*" + regexPattern + ".*";
        
        try {
            std::regex re(regexPattern, std::regex_constants::icase);
            if (std::regex_search(processName, re)) {
                return true;
            }
        } catch (const std::exception &e) {
            WARN(logger, "Regex match error: " << e.what());
        }
    }
    
    return false;
}

int SoftDomainTune::GetContainerCpuQuota(const Container &container)
{
    // 如果cfs_quota_us为-1，表示没有配额限制
    if (container.cfs_quota_us == -1) {
        return -1;
    }
    
    // 计算CPU配额：cpu.cfs_quota_us / cpu.cfs_period_us
    if (container.cfs_period_us > 0) {
        return static_cast<int>(container.cfs_quota_us / container.cfs_period_us);
    }
    
    return 0;
}

int SoftDomainTune::GetContainerNumaNode(const Container &container)
{
    // 从cpuset.cpus判断是否绑定NUMA节点
    if (container.cpus.empty()) {
        return -1;  // 未绑定CPU
    }
    
    // 需要env数据准备好才能判断
    if (!envDataReady || cpu2Node.empty()) {
        return -1;
    }
    
    try {
        // 解析CPU范围，例如 "0-3,5,7-9" -> [0,1,2,3,5,7,8,9]
        std::vector<int> cpus = ParseRange(container.cpus);
        if (cpus.empty()) {
            return -1;
        }
        
        // 检查CPU数量是否<=单个NUMA的CPU个数

        if (static_cast<int>(cpus.size()) > cpuCntPerNuma) {
            // CPU数量超过单个NUMA的CPU个数，认为未绑定NUMA
            return -1;
        }
        
        // 检查这些CPU是否都在同一个NUMA节点上
        int numaNode = -1;
        for (int cpu : cpus) {
            if (cpu < 0 || cpu >= static_cast<int>(cpu2Node.size())) {
                return -1;  // CPU编号无效
            }
            
            int node = cpu2Node[cpu];
            if (node < 0 || node >= numaNum) {
                return -1;  // NUMA节点编号无效
            }
            
            if (numaNode == -1) {
                numaNode = node;  // 第一个CPU的NUMA节点
            } else if (numaNode != node) {
                // 发现不同NUMA节点的CPU，认为跨NUMA，未绑定
                return -1;
            }
        }
        
        // 所有CPU都在同一个NUMA节点上，且CPU数量<=单个NUMA的CPU个数
        return numaNode;
    } catch (const std::exception &e) {
        WARN(logger, "Failed to parse NUMA node from cpuset.cpus: " << container.cpus << ", error: " << e.what());
        return -1;
    }
}

void SoftDomainTune::InitConfigsCache()
{
    configsCache.clear();
    
    for (const auto &item : configItems) {
        configsCache[item.type].push_back(item);
    }
    
    INFO(logger, "Initialized configs cache: " 
          << configsCache[SoftDomainConfigType::DOCKER].size() << " docker configs, " 
          << configsCache[SoftDomainConfigType::PROCESS].size() << " process configs");
}

std::vector<DockerInfo*> SoftDomainTune::BuildPendingContainers(
    const std::vector<SoftDomainConfigItem> &dockerConfigs)
{
    std::vector<DockerInfo*> pendingContainers;
    DEBUG(logger, "Building pending containers list from " << dockerInfos.size() << " docker containers");
    
    for (auto &docker : dockerInfos) {
        const std::string &containerId = docker.first; 
        DockerInfo &dockerInfo = docker.second;
        if (!dockerInfo.dataReady || dockerInfo.processed) {
            continue;
        }

        // 如果dockerConfigs为空，所有docker都进行调优
        // 如果dockerConfigs不为空，只有匹配白名单的docker调优
        if (dockerConfigs.empty()) {
            dockerInfo.shouldTune = true;
            dockerInfo.configCpuNum = 0;
            INFO(logger, "Container " << containerId << " matched (no config, using default)");
        } else {
            // dockerConfigs不为空，检查是否匹配白名单
            for (const auto &config : dockerConfigs) {
                if (MatchContainerName(containerId, config.whitelist)) {
                    dockerInfo.shouldTune = true;
                    dockerInfo.configCpuNum = config.cpuNum;
                    INFO(logger, "Container " << containerId << " matched whitelist, configCpuNum=" << dockerInfo.configCpuNum);
                    break;
                }
            }
        }
        
        if (!dockerInfo.shouldTune) {
            dockerInfo.processed = true;
            continue;  // 不匹配白名单（仅在dockerConfigs不为空时才会发生）
        }

        dockerInfo.softDomainNrCpu = CalculateSoftDomainNrCpu(dockerInfo, cpuCntPerNuma);
        if (dockerInfo.softDomainNrCpu <= 0) {
            dockerInfo.processed = true;
            WARN(logger, "Container " << containerId << " soft domain nr cpu (" << dockerInfo.softDomainNrCpu << ") is invalid, skip");
            continue;
        }

        pendingContainers.push_back(&dockerInfo);
    }
    
    // 按soft_domain的CPU配额排序，大的在前
    std::sort(pendingContainers.begin(), pendingContainers.end(),
              [](const DockerInfo* a, const DockerInfo* b) {
                  return a->softDomainNrCpu > b->softDomainNrCpu;
              });
    DEBUG(logger, "Built pending containers list: " << pendingContainers.size() << " to configure");
    return pendingContainers;
}

int SoftDomainTune::CalculateSoftDomainNrCpu(const DockerInfo &dockerInfo, int cpuCountPerNuma)
{
    int cpuQuota = GetContainerCpuQuota(dockerInfo.container);
    int configCpuNum = dockerInfo.configCpuNum;

    if (cpuQuota <= 0 || cpuQuota > cpuCountPerNuma) {
        WARN(logger, "Container " << dockerInfo.id << " cpu quota (" << cpuQuota << ") is invalid, should be in range [1, " << cpuCountPerNuma << "], skip");
        return 0;  // 不处理，配额应该在numa范围内
    }

    if (configCpuNum > 0 && configCpuNum <= cpuCountPerNuma) {
        DEBUG(logger, "Container " << dockerInfo.id << " using config cpu_num=" << configCpuNum << " (original quota=" << cpuQuota << ")");
        return configCpuNum;
    }

    return cpuQuota;
}

bool SoftDomainTune::ConfigureContainerToNuma(const std::string &containerId, int numaNode, int cpuQuota)
{
    std::string cgroupPath = "/sys/fs/cgroup/cpu/docker/" + containerId;
    
    if (!CreateCgroupDir(cgroupPath)) {
        ERROR(logger, "Failed to create cgroup directory for container " << containerId);
        return false;
    }
    
    // 设置cpu.soft_domain_nr_cpu
    std::string cpuNumStr = std::to_string(cpuQuota);
    if (!WriteValueToFile(cgroupPath + "/cpu.soft_domain_nr_cpu", cpuNumStr)) {         
        ERROR(logger, "Failed to set cpu.soft_domain_nr_cpu=" << cpuQuota 
              << " for container " << containerId);
        return false;
    }
    
    // 设置cpu.soft_domain（NUMA节点编号从1开始）
    std::string numaStr = std::to_string(numaNode + 1);
    if (!WriteValueToFile(cgroupPath + "/cpu.soft_domain", numaStr)) {
        ERROR(logger, "Failed to set cpu.soft_domain=" << (numaNode + 1) 
              << " for container " << containerId);
        return false;
    }
    
    // 更新dockerInfos中的绑定信息和配额
    auto &info = dockerInfos[containerId];
    info.softDomainNuma = numaNode + 1;
    numaRemainingQuota[numaNode] -= cpuQuota;
    
    // 获取并打印docker名称
    INFO(logger, "Successfully configured docker " << containerId
        << " (" << info.name << ") to NUMA " << numaNode
        << " with CPU quota " << cpuQuota);
    
    
    return true;
}

void SoftDomainTune::ProcessNumaBoundContainers(std::vector<DockerInfo*> &pendingContainers)
{
    DEBUG(logger, "Processing NUMA-bound containers");
    
    for (auto *info : pendingContainers) {
        if (info->boundNumaNode < 0 || info->boundNumaNode >= numaNum) {
            continue;  // 未绑定NUMA或NUMA节点无效
        }
        if (info->processed) {
            continue;
        }
        int numaNode = info->boundNumaNode;
        
        // 已绑定NUMA，优先处理
        int softDomainNrCpu = info->softDomainNrCpu; // 前面已经保证值在numa范围内
        
        // 检查NUMA节点是否有足够配额
        if (numaRemainingQuota[numaNode] < softDomainNrCpu) {
            WARN(logger, "Container " << info->id << " soft domain nr cpu (" 
                  << softDomainNrCpu << ") exceeds remaining quota (" 
                  << numaRemainingQuota[numaNode] << ") on NUMA " << numaNode << ", skip");
            continue;
        }   
        
        ConfigureContainerToNuma(info->id, numaNode, softDomainNrCpu);
        info->processed = true;
    }
}

void SoftDomainTune::ProcessUnboundContainers(std::vector<DockerInfo*> &pendingContainers)
{    
    INFO(logger, "Processing unbound containers");
    
    for (auto *info : pendingContainers) {
        // 跳过已处理的/NUMA绑定的容器
        if (info->processed || info->IsOptimized() || info->boundNumaNode >= 0) {
            continue;
        }

        int softDomainNrCpu = info->softDomainNrCpu;

        // 找到空闲配额最多的NUMA
        int bestNuma = FindBestNumaNode(softDomainNrCpu);
        if (bestNuma < 0) {
            WARN(logger, "Container " << info->id << " cpu quota ("
                << softDomainNrCpu << ") cannot fit in any NUMA, skip");
            continue;
        }

        ConfigureContainerToNuma(info->id, bestNuma, softDomainNrCpu);
        info->processed = true;
    }
}

int SoftDomainTune::FindBestNumaNode(int requiredQuota)
{
    int maxQuotaNuma = -1;
    int maxQuota = -1;
    
    for (int i = 0; i < numaNum; i++) {
        if (numaRemainingQuota[i] > maxQuota) {
            maxQuota = numaRemainingQuota[i];
            maxQuotaNuma = i;
        }
    }
    
    // 检查NUMA是否有足够配额
    if (maxQuotaNuma < 0 || maxQuota < requiredQuota) {
        INFO(logger, "No suitable NUMA node found: required=" << requiredQuota 
              << ", max available=" << maxQuota);
        return -1;
    }
    
    INFO(logger, "Found best NUMA node: " << maxQuotaNuma 
          << " with remaining quota " << maxQuota);
    return maxQuotaNuma;
}

void SoftDomainTune::ConfigureDockerSoftDomain()
{
    DEBUG(logger, "Starting docker soft domain configuration");
    
    // 使用缓存的docker配置项（在Enable时已初始化）
    const auto &dockerConfigs = configsCache[SoftDomainConfigType::DOCKER];
    // 构建待配置的容器列表
    std::vector<DockerInfo*> pendingContainers = BuildPendingContainers(dockerConfigs);
    
    if (pendingContainers.empty()) {
        DEBUG(logger, "No containers to configure");
        return;
    }
    
    // 优先处理已绑定NUMA的容器
    ProcessNumaBoundContainers(pendingContainers);
    
    // 处理未绑定NUMA的容器
    ProcessUnboundContainers(pendingContainers);
    
    DEBUG(logger, "Docker soft domain configuration completed");
}

void SoftDomainTune::ConfigureProcessSoftDomain()
{
    // 使用缓存的process配置项（在Enable时已初始化）
    const auto &processConfigs = configsCache[SoftDomainConfigType::PROCESS];
    if (processConfigs.empty()) {
        return;  // 没有进程配置
    }
    if (processInfos.empty() || !envDataReady) {
        return;
    }

    // 对每个配置项处理
    for (const auto &config : processConfigs) {
        if (config.cpuNum <= 0) {
            continue;
        }
        
        int cpuNum = config.cpuNum;
        
        // 计算需要创建的cgroup个数：系统cpu个数 / 进程配置的cpu个数
        int cgroupCount = cpuNumConfig / cpuNum;
        if (cgroupCount <= 0) {
            continue;
        }
        
        // 每个numa分配 cgroupCount / numaNum 个cgroup
        int cgroupsPerNuma = cgroupCount / numaNum;
        if (cgroupsPerNuma <= 0) {
            cgroupsPerNuma = 1;
        }
        
        // 获取或创建该cpuNum的cgroup列表
        auto &cgroups = processCgroupsByCpuNum[cpuNum];
        
        // 创建cgroup（如果还没有创建）
        if (cgroups.empty()) {
            for (int i = 0; i < cgroupCount; i++) {
                int numaNode = i / cgroupsPerNuma;
                if (numaNode >= numaNum) {
                    numaNode = numaNum - 1;
                }
                
                // 使用cpuNum作为cgroup名称的一部分，以区分不同的配置
                std::string cgroupName = "soft_domain_cpu" + std::to_string(cpuNum) + "_" + std::to_string(i);
                std::string cgroupPath = "/sys/fs/cgroup/cpu/" + cgroupName;
                
                if (CreateCgroupDir(cgroupPath)) {
                    // 设置cpu.soft_domain_nr_cpu
                    std::string cpuNumStr = std::to_string(cpuNum);
                    if (WriteValueToFile(cgroupPath + "/cpu.soft_domain_nr_cpu", cpuNumStr)) {
                        // 设置cpu.soft_domain（NUMA节点编号从1开始）
                        std::string numaStr = std::to_string(numaNode + 1);
                        if (WriteValueToFile(cgroupPath + "/cpu.soft_domain", numaStr)) {
                            cgroups.push_back(cgroupPath);
                            INFO(logger, "Created process cgroup: " << cgroupPath 
                                 << " for NUMA " << numaNode << " (cpuNum=" << cpuNum << ")");
                        }
                    }
                }
            }
        }
        
        // 如果cgroup创建失败，跳过分配
        if (cgroups.empty()) {
            WARN(logger, "No process cgroups created for cpuNum=" << cpuNum << ", skip process allocation");
            continue;
        }
        
        // 将匹配的进程分配到cgroup（相同进程名尽量在各cgroup数量相等）
        for (const auto &process : processInfos) {
            int pid = process.first;
            const std::string &processName = process.second;
            
            if (!MatchProcessName(processName, config.whitelist)) {
                continue;  // 不匹配白名单
            }
            
            // 如果进程已经在某个配置的cgroup中，跳过（一个进程只能属于一个配置）
            if (processPidToCgroupInfo.find(pid) != processPidToCgroupInfo.end()) {
                continue;
            }
            
            // 初始化该cpuNum下该进程名在各cgroup的计数（如果还没有）
            auto &nameToCounts = processNameToCgroupCountByCpuNum[cpuNum];
            if (nameToCounts.find(processName) == nameToCounts.end()) {
                nameToCounts[processName] = std::vector<int>(cgroups.size(), 0);
            }
            
            // 找到该进程名在哪个cgroup中数量最少
            auto &counts = nameToCounts[processName];
            if (counts.empty()) {
                continue;  // 安全保护
            }
            
            int minCount = counts[0];
            int minCgroupIdx = 0;
            for (size_t i = 1; i < counts.size(); i++) {
                if (counts[i] < minCount) {
                    minCount = counts[i];
                    minCgroupIdx = i;
                }
            }
            
            // 将进程添加到数量最少的cgroup
            if (minCgroupIdx >= 0 && minCgroupIdx < static_cast<int>(cgroups.size())) {
                std::string cgroupPath = cgroups[minCgroupIdx];
                std::string tasksPath = cgroupPath + "/tasks";
                std::string pidStr = std::to_string(pid);
                if (WriteValueToFile(tasksPath, pidStr)) {
                    INFO(logger, "Added process " << processName << " (pid=" << pid 
                          << ") to cgroup " << cgroupPath << " (cpuNum=" << cpuNum 
                          << ", cgroup index " << minCgroupIdx << ")");
                    
                    // 更新映射和计数
                    processPidToCgroupInfo[pid] = std::make_pair(cpuNum, minCgroupIdx);
                    counts[minCgroupIdx]++;
                }
            }
        }
    }
}

void SoftDomainTune::CleanupProcessCgroups()
{
    const std::string rootCgroupTasksPath = "/sys/fs/cgroup/cpu/tasks";
    
    // 遍历所有创建的进程cgroup
    for (auto &pair : processCgroupsByCpuNum) {
        const std::vector<std::string> &cgroups = pair.second;
        
        for (const std::string &cgroupPath : cgroups) {
            std::string tasksPath = cgroupPath + "/tasks";
            
            // 读取cgroup中的所有进程PID
            std::vector<int> pids;
            std::ifstream tasksFile(tasksPath);
            if (tasksFile.is_open()) {
                std::string line;
                while (std::getline(tasksFile, line)) {
                    if (!line.empty()) {
                        try {
                            int pid = std::stoi(line);
                            pids.push_back(pid);
                        } catch (const std::exception &e) {
                            WARN(logger, "Failed to parse PID from tasks file: " << line);
                        }
                    }
                }
                tasksFile.close();
            } else {
                WARN(logger, "Failed to open tasks file: " << tasksPath);
            }
            
            // 将进程移到根cgroup
            for (int pid : pids) {
                std::string pidStr = std::to_string(pid);
                if (WriteValueToFile(rootCgroupTasksPath, pidStr)) {
                    DEBUG(logger, "Moved process " << pid << " from " << cgroupPath << " to root cgroup");
                } else {
                    WARN(logger, "Failed to move process " << pid << " from " << cgroupPath << " to root cgroup");
                }
            }
            
            // 等待一下，确保系统有时间处理移动操作
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 再次检查tasks文件是否为空（确保所有进程都已移出）
            std::ifstream checkTasksFile(tasksPath);
            bool isEmpty = true;
            if (checkTasksFile.is_open()) {
                std::string line;
                if (std::getline(checkTasksFile, line) && !line.empty()) {
                    isEmpty = false;
                }
                checkTasksFile.close();
            }
            
            if (!isEmpty) {
                WARN(logger, "Cgroup " << cgroupPath << " still has tasks, skip deletion");
                continue;
            }
            
            // 删除cgroup目录
            if (rmdir(cgroupPath.c_str()) == 0) {
                INFO(logger, "Deleted cgroup directory: " << cgroupPath);
            } else {
                WARN(logger, "Failed to delete cgroup directory: " << cgroupPath << ", error: " << strerror(errno));
            }
        }
    }
}


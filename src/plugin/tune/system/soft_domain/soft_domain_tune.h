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
#ifndef SOFT_DOMAIN_TUNE_H
#define SOFT_DOMAIN_TUNE_H

#include "oeaware/interface.h"
#include "oeaware/data/docker_data.h"
#include "oeaware/data/thread_info.h"
#include "oeaware/data/env_data.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>
#include <yaml-cpp/yaml.h>

namespace oeaware {

/**
 * @brief 配置类型枚举
 */
enum class SoftDomainConfigType {
    DOCKER,   // docker类型
    PROCESS   // process类型
};

/**
 * @brief 配置项结构体
 */
struct SoftDomainConfigItem {
    SoftDomainConfigType type;           // 配置类型
    std::vector<std::string> whitelist;  // 白名单（正则表达式）
    int cpuNum = 0;                      // CPU配额（0表示未配置）
};

struct DockerInfo {
    bool shouldTune = false; // 是否需要调优
    bool dataReady = false; // 数据更新完，未更新完的数据不进行处理
    bool processed = false; // 是否已经处理过，通常一个docker处理一次后不在修改
    std::string id; // docker 本身的id
    std::string name; // docker 的名称
    Container container; // docker 本身的容器信息
    int softDomainNrCpu = 0; // soft_domain的CPU配额
    int configCpuNum = 0; // 配置文件中的配额，如果配置文件未配置，则为0.
    int boundNumaNode = -1;   // docker 本身的NUMA节点，-1 表示未绑定NUMA或者绑定超过NUMA范围，0-numaNum-1 表示绑定的NUMA节点
    // soft_domain 的 NUMA节点，boundNumaNode>=0 时，softDomainNuma = soft_domain的NUMA节点，boundNumaNode+1 , 0 表示未配置，1-numaNum 表示配置的NUMA节点
    int softDomainNuma = 0;
    bool IsOptimized() const { return softDomainNuma > 0; }
};

/**
 * @brief SoftDomainTune类，用于使能SOFT_DOMAIN调度特性
 */
class SoftDomainTune : public Interface {
public:
    SoftDomainTune();
    ~SoftDomainTune() override = default;
    
    // Interface实现
    Result OpenTopic(const Topic &topic) override;
    void CloseTopic(const Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    // 更新docker数据
    void UpdateDockerData(const DataList &dataList);
    // 更新线程数据
    void UpdateThreadData(const DataList &dataList);
    // 更新环境信息数据
    void UpdateEnvData(const DataList &dataList);
    // 加载配置文件
    bool LoadConfig();
    // 解析配置文件
    bool ParseConfig(const YAML::Node &node);
    // 从系统直接获取NUMA和CPU信息（用于Enable时env数据未准备好的情况）
    bool InitEnvInfoFromSystem();
    // 校验配置
    bool ValidateConfig();
    // 创建cgroup目录
    bool CreateCgroupDir(const std::string &path);
    // 配置Docker的soft domain
    void ConfigureDockerSoftDomain();
    // 配置进程的soft domain
    void ConfigureProcessSoftDomain();
    // 获取容器名称（通过docker inspect命令）
    std::string GetContainerName(const std::string &containerId);
    // 匹配容器名称（支持通配符）
    bool MatchContainerName(const std::string &containerId, const std::vector<std::string> &whitelist);
    // 匹配进程名称（支持通配符）
    bool MatchProcessName(const std::string &processName, const std::vector<std::string> &whitelist);
    // 获取容器的CPU配额
    int GetContainerCpuQuota(const Container &container);
    // 获取容器绑定的NUMA节点
    int GetContainerNumaNode(const Container &container);
    
    // Docker配置相关的辅助函数
    // 初始化缓存的配置项（在Enable时调用）
    void InitConfigsCache();
    // 构建待配置的容器列表
    std::vector<DockerInfo*> BuildPendingContainers(const std::vector<SoftDomainConfigItem> &dockerConfigs);
    // 计算soft_domain的CPU配额
    int CalculateSoftDomainNrCpu(const DockerInfo &dockerInfo, int cpuCountPerNuma);
    // 配置容器到指定的NUMA节点
    bool ConfigureContainerToNuma(const std::string &containerId, int numaNode, int cpuQuota);
    // 处理已绑定NUMA的容器
    void ProcessNumaBoundContainers(std::vector<DockerInfo*> &pendingContainers);
    // 处理未绑定NUMA的容器
    void ProcessUnboundContainers(std::vector<DockerInfo*> &pendingContainers);
    // 找到空闲配额最多的NUMA节点
    int FindBestNumaNode(int requiredQuota);
    // 清理进程cgroup（将进程移到根cgroup并删除cgroup目录）
    void CleanupProcessCgroups();

    const int defaultPeriod = 1000;
    const int defaultPriority = 2;
    const std::string configPath = "/etc/oeAware/plugin/soft_domain.yaml";
    
    // 订阅的topic列表
    std::vector<Topic> subscribeTopics;
    
    // 存储docker信息，key为docker id，value为DockerInfo信息
    std::unordered_map<std::string, DockerInfo> dockerInfos;
    
    // 存储线程信息，key为pid，value为线程名称
    std::unordered_map<int, std::string> processInfos;
    
    // 配置项列表
    std::vector<SoftDomainConfigItem> configItems;
    // 按类型缓存的配置项（只在Enable时初始化，避免每次Run时重新收集）
    std::unordered_map<SoftDomainConfigType, std::vector<SoftDomainConfigItem>> configsCache;
    
    // 环境信息（从env_info_collector订阅）
    bool envDataReady = false;           // env数据是否已准备好
    int numaNum = 0;                     // NUMA节点数量
    int cpuNumConfig = 0;                // CPU配置数量
    std::vector<int> cpu2Node;           // CPU到NUMA节点的映射
    int cpuCntPerNuma = 0;               // 每个NUMA节点的CPU数量 估算值
    
    // SOFT_DOMAIN相关
    std::string schedFeaturesPath;      // sched_features文件路径

    // Docker配置相关
    std::vector<int> numaRemainingQuota;  // 每个NUMA节点的剩余配额
    
    // 进程配置相关（按cpuNum区分不同的配置）
    std::unordered_map<int, std::vector<std::string>> processCgroupsByCpuNum;  // cpuNum -> cgroup路径列表
    std::unordered_map<int, std::pair<int, int>> processPidToCgroupInfo;  // pid -> (cpuNum, cgroupIndex)（记录每个进程在哪个cpuNum配置的哪个cgroup）
    std::unordered_map<int, std::unordered_map<std::string, std::vector<int>>> processNameToCgroupCountByCpuNum;  // cpuNum -> (进程名 -> 每个cgroup中该进程名的数量)
};

} // namespace oeaware

#endif // SOFT_DOMAIN_TUNE_H


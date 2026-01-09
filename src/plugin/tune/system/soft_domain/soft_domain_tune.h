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
    std::string cpuNum;                  // CPU配额（字符串格式）
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
    // 获取单个NUMA节点的CPU数量
    int GetCpuCountPerNuma();
    // 校验配置
    bool ValidateConfig();
    
    const int defaultPeriod = 1000;
    const int defaultPriority = 2;
    const std::string configPath = "/etc/oeAware/plugin/soft_domain.yaml";
    
    // 订阅的topic列表
    std::vector<Topic> subscribeTopics;
    
    // 存储docker信息，key为docker id，value为Container信息
    std::unordered_map<std::string, Container> dockerContainers;
    
    // 存储线程信息，key为tid，value为线程名称
    std::unordered_map<int, std::string> threadInfos;
    
    // 配置项列表
    std::vector<SoftDomainConfigItem> configItems;
    
    // 环境信息（从env_info_collector订阅）
    bool envDataReady = false;           // env数据是否已准备好
    int numaNum = 0;                     // NUMA节点数量
    int cpuNumConfig = 0;                // CPU配置数量
    std::vector<int> cpu2Node;           // CPU到NUMA节点的映射
    std::vector<int> numaCpuCounts;      // 每个NUMA节点的CPU数量
};

} // namespace oeaware

#endif // SOFT_DOMAIN_TUNE_H


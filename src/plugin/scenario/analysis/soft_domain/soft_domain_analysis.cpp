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
#include "soft_domain_analysis.h"
#include "oeaware/utils.h"
#include "oeaware/data/analysis_data.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <numa.h>
#include <unistd.h>

namespace oeaware {

const int MS_PER_SEC = 1000;
const int DEFAULT_ANALYSIS_TIME = 10; // 默认分析时间10秒

SoftDomainAnalysis::SoftDomainAnalysis()
{
    name = OE_SOFT_DOMAIN_ANALYSIS;
    version = "1.0.0";
    period = ANALYSIS_TIME_PERIOD;
    priority = 1;
    type = SCENARIO;
    
    // 订阅docker和环境信息数据
    subscribeTopics.emplace_back(oeaware::Topic{OE_DOCKER_COLLECTOR, OE_DOCKER_COLLECTOR, ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_ENV_INFO, "static", ""});
    
    // 注册支持的topic
    for (const auto &topicStr : topicStrs) {
        supportTopics.emplace_back(Topic{name, topicStr, ""});
        topicCtl[topicStr].topicName = topicStr;
    }
}

Result SoftDomainAnalysis::OpenTopic(const oeaware::Topic &topic)
{
    if (topicCtl.count(topic.topicName) == 0) {
        return Result(FAILED, "topic not register");
    }
    if (topicCtl[topic.topicName].isOpen) {
        return Result(FAILED, "topic already opened");
    }
    
    auto paramsMap = GetKeyValueFromString(topic.params);
    if (paramsMap.count("t")) {
        topicCtl[topic.topicName].analysisTime = atoi(paramsMap["t"].data());
        if (topicCtl[topic.topicName].analysisTime <= 0) {
            topicCtl[topic.topicName].analysisTime = DEFAULT_ANALYSIS_TIME;
        }
    }
    
    topicCtl[topic.topicName].beginTime = std::chrono::high_resolution_clock::now();
    topicCtl[topic.topicName].isOpen = true;
    topicCtl[topic.topicName].openParams = topic.params;
    topicCtl[topic.topicName].hasPublished = false;
    
    return Result(OK);
}

void SoftDomainAnalysis::CloseTopic(const oeaware::Topic &topic)
{
    if (topicCtl.count(topic.topicName) == 0) {
        return;
    }
    if (!topicCtl[topic.topicName].isOpen) {
        return;
    }
    topicCtl[topic.topicName].isOpen = false;
    topicCtl[topic.topicName].analysisTime = DEFAULT_ANALYSIS_TIME;
    topicCtl[topic.topicName].openParams = "";
    topicCtl[topic.topicName].hasPublished = false;
}

Result SoftDomainAnalysis::Enable(const std::string &param)
{
    (void)param;
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }
    return Result(OK);
}

void SoftDomainAnalysis::Disable()
{
    AnalysisResultItemFree(&analysisResultItem);
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    dockerContainers.clear();
    envDataReady = false;
    numaNum = 0;
    cpuNumConfig = 0;
    cpuCountPerNuma = 0;
    cpu2Node.clear();
}

void SoftDomainAnalysis::UpdateData(const DataList &dataList)
{
    Topic topic{dataList.topic.instanceName, dataList.topic.topicName, dataList.topic.params};
    
    if (topic.instanceName == OE_DOCKER_COLLECTOR && topic.topicName == OE_DOCKER_COLLECTOR) {
        // 更新docker数据
        for (uint64_t i = 0; i < dataList.len; i++) {
            auto *container = static_cast<Container*>(dataList.data[i]);
            if (container == nullptr) {
                continue;
            }
            dockerContainers[container->id] = *container;
        }
    } else if (topic.instanceName == OE_ENV_INFO && topic.topicName == "static") {
        // 更新环境信息
        if (dataList.len > 0 && dataList.data[0] != nullptr) {
            auto *envStaticInfo = static_cast<EnvStaticInfo*>(dataList.data[0]);
            if (envStaticInfo != nullptr && !envDataReady) {
                numaNum = envStaticInfo->numaNum;
                cpuNumConfig = envStaticInfo->cpuNumConfig;
                cpu2Node.clear();
                cpu2Node.resize(cpuNumConfig);
                for (int i = 0; i < cpuNumConfig; i++) {
                    cpu2Node[i] = envStaticInfo->cpu2Node[i];
                }
                // 计算每个NUMA节点的CPU数量
                if (numaNum > 0) {
                    cpuCountPerNuma = cpuNumConfig / numaNum;
                }
                envDataReady = true;
            }
        }
    }
}

void SoftDomainAnalysis::Run()
{
    // 分析docker信息
    Analysis();
    
    // 检查是否需要发布结果
    auto now = std::chrono::high_resolution_clock::now();
    for (auto &it : topicCtl) {
        auto &info = it.second;
        if (!info.isOpen || info.hasPublished) {
            continue;
        }
        
        int curTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.beginTime).count();
        if (curTimeMs / MS_PER_SEC >= info.analysisTime) {
            GeneratePublishData();
            
            DataList dataList;
            SetDataListTopic(&dataList, name, info.topicName, info.openParams);
            dataList.len = 1;
            dataList.data = new void*[dataList.len];
            dataList.data[0] = &analysisResultItem;
            Publish(dataList, false);
            
            info.hasPublished = true;
        }
    }
}

void SoftDomainAnalysis::Analysis()
{
    analysisResult = AnalysisResult();
    
    if (!envDataReady || cpuCountPerNuma <= 0) {
        return;
    }
    
    analysisResult.totalDockerCount = dockerContainers.size();
    analysisResult.recommendedDockerInfos.clear();
    
    for (const auto &pair : dockerContainers) {
        const std::string &containerId = pair.first;
        const Container &container = pair.second;
        
        DockerAnalysisInfo info;
        info.containerId = containerId;
        info.containerName = GetContainerName(containerId);
        info.cpuQuota = GetContainerCpuQuota(container);

        // 判断是否推荐：配额存在且小于1个numa的CPU数量
        // cpuQuota > 0 表示配额存在，cpuQuota < cpuCountPerNuma 表示小于1个numa
        if (info.cpuQuota > 0 && info.cpuQuota < cpuCountPerNuma) {
            info.shouldRecommend = true;
            analysisResult.recommendedDockerInfos.push_back(info);
        }
    }
    
    analysisResult.recommendedDockerCount = analysisResult.recommendedDockerInfos.size();
    
    // 判断是否需要调优：存在推荐的docker，且系统支持soft_domain
    analysisResult.shouldTune = (analysisResult.recommendedDockerCount > 0 && IsSoftDomainSupported());
}

void SoftDomainAnalysis::GeneratePublishData()
{
    std::vector<std::vector<std::string>> metrics;
    std::vector<int> type;
    std::string conclusion;
    std::vector<std::string> suggestionItem;
    
    conclusion = "soft_domain analysis:\n";
    conclusion += "  Total docker containers: " + std::to_string(analysisResult.totalDockerCount) + "\n";
    conclusion += "  Recommended docker containers (quota exists and < " + std::to_string(cpuCountPerNuma) + " CPUs per NUMA): " + 
                 std::to_string(analysisResult.recommendedDockerCount) + "\n";
    
    if (analysisResult.totalDockerCount == 0) {
        conclusion += "  No docker containers found, no tuning needed.\n";
    } else if (!IsSoftDomainSupported()) {
        conclusion += "  System does not support SOFT_DOMAIN feature.\n";
    } else if (analysisResult.recommendedDockerCount == 0) {
        conclusion += "  No docker containers meet the recommendation criteria (quota exists and < " + 
                     std::to_string(cpuCountPerNuma) + " CPUs per NUMA).\n";
    } else {
        conclusion += "  Recommended docker containers for soft_domain scheduling: ";
        
        // 按CPU配额排序，显示所有推荐的docker
        std::vector<DockerAnalysisInfo> sortedInfos = analysisResult.recommendedDockerInfos;
        std::sort(sortedInfos.begin(), sortedInfos.end(), 
                  [](const DockerAnalysisInfo &a, const DockerAnalysisInfo &b) {
                      return a.cpuQuota > b.cpuQuota;
                  });
        
        int showCount = std::min(10, static_cast<int>(sortedInfos.size())); // 最多显示10个
        std::string dockerList;
        for (int i = 0; i < showCount; i++) {
            const auto &info = sortedInfos[i];
            if (!dockerList.empty()) {
                dockerList += ",";
            }
            dockerList += info.containerName + "(" + std::to_string(info.cpuQuota) + ")";
            
            type.emplace_back(DATA_TYPE_CPU);
            metrics.emplace_back(std::vector<std::string>{
                info.containerName,
                std::to_string(info.cpuQuota),
                ""
            });
        }
        conclusion += dockerList + "\n";
    }
    
    if (analysisResult.shouldTune) {
        suggestionItem.emplace_back("Use soft_domain_tune plugin to optimize docker containers");
        suggestionItem.emplace_back("oeawarectl -e soft_domain_tune");
        suggestionItem.emplace_back("Configure /etc/oeAware/plugin/soft_domain.yaml with docker whitelist and cpu_num");
    } else if (analysisResult.totalDockerCount > 0 && analysisResult.recommendedDockerCount == 0) {
        suggestionItem.emplace_back("No docker containers meet the recommendation criteria");
    }
    
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
}

int SoftDomainAnalysis::GetContainerCpuQuota(const Container &container)
{
    if (container.cfs_quota_us == -1) {
        return -1; // 无限制
    }
    if (container.cfs_period_us > 0) {
        return static_cast<int>(container.cfs_quota_us / container.cfs_period_us);
    }
    return 0;
}

int SoftDomainAnalysis::GetContainerNumaNode(const Container &container)
{
    if (container.cpus.empty() || !envDataReady || cpu2Node.empty()) {
        return -1;
    }
    
    try {
        std::vector<int> cpus = ParseRange(container.cpus);
        if (cpus.empty()) {
            return -1;
        }
        
        int numaNode = -1;
        for (int cpu : cpus) {
            if (cpu < 0 || cpu >= static_cast<int>(cpu2Node.size())) {
                return -1;
            }
            int node = cpu2Node[cpu];
            if (node < 0 || node >= numaNum) {
                return -1;
            }
            if (numaNode == -1) {
                numaNode = node;
            } else if (numaNode != node) {
                return -1; // 跨NUMA
            }
        }
        return numaNode;
    } catch (...) {
        return -1;
    }
}

bool SoftDomainAnalysis::IsSoftDomainSupported()
{
    std::string schedFeaturesPath;
    std::vector<std::string> features;
    if (!ReadSchedFeatures(schedFeaturesPath, features)) {
        return false;
    }
    for (const auto &feature : features) {
        if (feature.find("SOFT_DOMAIN") != std::string::npos) {
            return true;
        }
    }
    return false;
}

std::string SoftDomainAnalysis::GetContainerName(const std::string &containerId)
{
    std::string command = "docker inspect --format='{{.Name}}' " + containerId + " 2>/dev/null";
    std::string result;
    if (ExecCommand(command, result)) {
        result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
        result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
        if (!result.empty() && result[0] == '/') {
            result = result.substr(1);
        }
        return result;
    }
    return containerId.substr(0, 12); // 返回短ID
}

} // namespace oeaware


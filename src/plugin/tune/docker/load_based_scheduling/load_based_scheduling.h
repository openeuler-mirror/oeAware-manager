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
#ifndef LOAD_BASED_SCHEDULING_H
#define LOAD_BASED_SCHEDULING_H
#include <unordered_set>
#include <oeaware/interface.h>
#include <oeaware/data/docker_data.h>

namespace oeaware {
constexpr int PERIOD = 1000;
constexpr int PRIORITY = 2;
class LoadBasedScheduling : public Interface {
public:
    LoadBasedScheduling();
    ~LoadBasedScheduling() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
private:
    struct ContainerInfo {
        int containerNum; // 容器数量
        std::vector<int> cpus; // 容器绑定的cpu列表
        int usedIndex; // 已经使用的cpu索引
    };
    struct ContainerCpuInfo {
        std::string id;
        int64_t cycles;
        int64_t totLoad;
        double cpuLoad;
        std::string cpus;
        std::string originPreferredCpu;
        std::unordered_set<uint64_t> tasks; // 任务列表
    };
    Result ReadConfig(const std::string &path);
    void InitCpuCycles(int cpuNum);
    void UpdateDockerData(const DataList &dataList);
    void UpdateNumaInfo(const DataList &dataList);
    void UpdatePmuData(const DataList &dataList);
    bool IsHighLoad(const ContainerCpuInfo &container, int cpuNum);
    std::vector<oeaware::Topic> subscribeTopics;
    std::vector<int64_t> maxCycles; // 每个cpu的最大周期
    std::unordered_set<std::string> tuneContainers; // 准备调度的容器
    std::unordered_map<int32_t, std::string> pids; // 容器pid
    std::unordered_set<int> usedCpus; // 已经使用的cpu
    std::unordered_map<std::string, ContainerCpuInfo> containers;
    std::unordered_map<std::string, int> containerNuma; // 容器对应的numa节点
    std::unordered_map<int, int> cpuNuma; // cpu对应的numa节点
    std::unordered_map<int, ContainerInfo> numaContainers; // numa节点的容器信息
    std::string configPath = oeaware::DEFAULT_PLUGIN_CONFIG_PATH + "/load_based_scheduling.yaml";
    const int unlimit = -1;
    double highLoadThreshold = 0.5; // 高负载阈值
};
}
#endif

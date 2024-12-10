/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include "cpu_burst.h"
#include <fstream>
#include <iostream>
#include <unistd.h>
#include "oeaware/data/pmu_counting_data.h"

constexpr double NINETY_PERCENT = 0.9;
constexpr int MILLISECONDS_IN_SECOND = 1000;

static void SetCfsBurstUs(const std::string &id, int cfsBurstUs)
{
    if (id == "null") {
        return;
    }
    std::string fileName = "/sys/fs/cgroup/cpu/docker/" + id + "/cpu.cfs_burst_us";
    std::ofstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "failed to open " << fileName << std::endl;
        return;
    }

    file << cfsBurstUs << '\n';
    file.close();
    return;
}

static uint64_t GetCpuCycles(int cpu)
{
    std::string freq_path = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cpufreq/scaling_cur_freq";
    std::ifstream freq_file(freq_path);

    if (!freq_file.is_open()) {
        return 0;
    }

    uint64_t freq;
    freq_file >> freq;
    freq_file.close();

    if (freq_file.fail()) {
        return 0;
    }

    return freq * 1000; // 1000: kHz to Hz
}

bool CpuBurst::Init()
{
    curSysCycles = 0;
    cpuNum = sysconf(_SC_NPROCESSORS_CONF);
    for (unsigned int i = 0; i < cpuNum; i++) {
        maxSysCycles += GetCpuCycles(i);
    }
    if (cpuNum <= 0 || maxSysCycles <= 0) {
        return false;
    }
    return true;
}

void CpuBurst::Update(const DataList &dataList)
{
    auto instance_name = dataList.topic.instanceName;
    auto topic_name = dataList.topic.topicName;
    if (instance_name == std::string("pmu_counting_collector") && topic_name == std::string("cycles")) {
        UpdatePmu(dataList);
    } else if (instance_name == std::string(OE_DOCKER_COLLECTOR) && topic_name == std::string(OE_DOCKER_COLLECTOR)) {
        UpdateDocker(dataList);
    }
}

void CpuBurst::UpdatePmu(const DataList &dataList)
{
    auto *tmp = (PmuCountingData*)dataList.data[0];
    for (int i = 0; i < tmp->len; i++) {
        if (tmp->pmuData[i].cpu < cpuNum) {
            curSysCycles += tmp->pmuData[i].count;
        }
    }
}

void CpuBurst::UpdateDocker(const DataList &dataList)
{
    std::unordered_map<std::string, Container*> collector;
    for (uint64_t i = 0; i < dataList.len; i++) {
        auto *tmp = (Container*)dataList.data[i];
        collector[tmp->id] = tmp;
    }

    for (const auto &container : collector) {
        std::string id = container.first;
        const auto &container_info = container.second;
        if (containers.find(id) == containers.end()) {
            containers[id].id = id;
            containers[id].cfs_burst_us_ori = container_info->cfs_burst_us;
        }
        containers[id].cfs_period_us = container_info->cfs_period_us;
        containers[id].cfs_quota_us = container_info->cfs_quota_us;
        containers[id].cfs_burst_us = container_info->cfs_burst_us;
    }

    // remove containers
    for (auto it = containers.begin(); it != containers.end();) {
        if (collector.find(it->first) == collector.end()) {
            it->second.Exit();
            it = containers.erase(it);
        } else {
            ++it;
        }
    }
}

void CpuBurst::Tune(int periodMs)
{
    if (curSysCycles > maxSysCycles * periodMs / MILLISECONDS_IN_SECOND * NINETY_PERCENT) {
        Exit();
        return;
    }
    for (auto &container : containers) {
        if (container.second.cfs_burst_us < container.second.cfs_quota_us) {
            SetCfsBurstUs(container.first, container.second.cfs_quota_us);
        }
    }
    curSysCycles = 0;
}

void CpuBurst::Exit()
{
    curSysCycles = 0;
    for (auto &container : containers) {
        container.second.Exit();
    }
}

void ContainerTune::Exit()
{
    SetCfsBurstUs(id, cfs_burst_us_ori);
}

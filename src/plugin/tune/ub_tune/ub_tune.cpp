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

#include <algorithm>
#include <fstream>
#include "interface.h"
#include "thread_info.h"
#include "ub_tune.h"

using namespace oeaware;

void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<ThreadTune>());
}

ThreadTune::ThreadTune() {
    name = "thread_tune";
    description = "";
    version = "1.0.0";
    period = 500;
    priority = 2;
    depTopic.instanceName = "thread_scenario";
    depTopic.topicName = "thread_scenario";
}

int ThreadTune::OpenTopic(const oeaware::Topic &topic) {
    (void)topic;
    return 0;
}

void ThreadTune::CloseTopic(const oeaware::Topic &topic) {
    (void)topic;
}

void ThreadTune::UpdateData(const oeaware::DataList &dataList) {
    int tid;
    // bind numa node0 by default.
    cpu_set_t *newMask = &nodes[DEFAULT_BIND_NODE].cpuMask;
    cpu_set_t currentMask;

    for (auto &data: dataList.data) {
        auto *tmp = (ThreadInfo*)data.get();
        if (std::find(keyThreadNames.begin(), keyThreadNames.end(), tmp->name) == keyThreadNames.end()) {
            continue;
        }
        tid = tmp->tid;
        sched_getaffinity(tid, sizeof(currentMask), &currentMask);
        // If currentMask is equal to newMask, no further action
        // is taken to avoid calls to sched_setaffinity redundantly.
        if (CPU_EQUAL(newMask, &currentMask)) {
            continue;
        }

        if (sched_setaffinity(tid, sizeof(*newMask), newMask) == 0) {
            bindTid.insert(tid);
        }
    }
}

int ThreadTune::Enable(const std::string &param) {
    (void)param;
    if (!isInit) {
        // The static data such as the number of CPUs and NUMA nodes
        // only needs to be assigned when enabling for the first time.
        cpuNum = numa_num_configured_cpus();
        maxNode = numa_max_node();
        initCpuMap();
        initNodeCpuMask();
        initAllCpumask();
        isInit = true;
    }

    ReadKeyThreads(CONFIG_PATH);
    return 0;
}

void ThreadTune::Disable() {
    cpu_set_t *mask = &allCpuMask;

    for(const auto &tid : bindTid) {
        sched_setaffinity(tid, sizeof(*mask), mask);
    }

    bindTid.clear();
    keyThreadNames.clear();
}

void ThreadTune::Run() {
}

void ThreadTune::ReadKeyThreads(const std::string &file_name)
{
    std::ifstream file(file_name);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        keyThreadNames.insert(line);
    }
    file.close();
}

void ThreadTune::initCpuMap()
{
    int nid;

    for (int i = 0; i < MAX_CPU; i++) {
        cpuNodeMap[i] = -1;
    }

    for (int cpu = 0; cpu < cpuNum; cpu++) {
        nid = numa_node_of_cpu(cpu);
        cpuNodeMap[cpu] = nid;
    }
}

void ThreadTune::initNodeCpuMask()
{
    int cpu_index = 0;

    for (int i = 0; i <= maxNode; i++) {
        Node *node = &nodes[i];
        node->id = i;
        CPU_ZERO(&node->cpuMask);

        for (int cpu = 0; cpu < cpuNum; cpu++) {
            if (cpuNodeMap[cpu] == node->id) {
                CPU_SET(cpu, &node->cpuMask);
                node->cpus[cpu_index++] = cpu;
            }
        }

        node->cpuNum = cpu_index;
    }
}

void ThreadTune::initAllCpumask()
{
    CPU_ZERO(&allCpuMask);
    for (int cpu = 0; cpu < cpuNum; cpu++) {
        CPU_SET(cpu, &allCpuMask);
    }
}
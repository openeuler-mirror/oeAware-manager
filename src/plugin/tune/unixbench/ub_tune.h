/******************************************************************************
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef THREAD_TUNE_H
#define THREAD_TUNE_H
#include <set>
#include <numa.h>
#include <sched.h>
#include "interface.h"
#define MAX_CPU 4096
#define MAX_NODE 8
#define DEFAULT_BIND_NODE 0

namespace oeaware {
struct Node {
    int id;
    cpu_set_t cpuMask;
    int cpus[MAX_CPU];
    int cpuNum;
};

class UnixBenchTune : public Interface{
public:
    UnixBenchTune();
    ~UnixBenchTune() override = default;
    int OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const oeaware::DataList &dataList) override;
    int Enable(const std::string &parma) override;
    void Disable() override;
    void Run() override;

private:
    Topic depTopic;
    int cpuNum = 0;
    int maxNode = 0;
    int cpuNodeMap[MAX_CPU]{};
    cpu_set_t allCpuMask{};
    Node nodes[MAX_NODE]{};
    std::set<int> bindTid;
    bool isInit = false;
    std::string CONFIG_PATH = "/usr/lib64/oeAware-plugin/thread_tune.conf";
    std::set<std::string> keyThreadNames;
    void ReadKeyThreads(const std::string &file_name);
    void initCpuMap();
    void initNodeCpuMask();
    void initAllCpumask();
};
}
#endif // !THREAD_TUNE_H

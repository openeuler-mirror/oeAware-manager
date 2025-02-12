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
#ifndef HARDIRQ_TUNE_H
#define HARDIRQ_TUNE_H

#include "oeaware/interface.h"
#include "libkperf/pmu.h"

struct RecNetQueue {
    uint64_t ts;
    uint64_t queueMapping;
    const void *skbaddr;
    std::string dev;
};

struct RecNetThreads {
    uint64_t ts;
    uint32_t core;
    const void *skbaddr;
};

namespace oeaware {
class NetHardIrq : public Interface{
public:
    NetHardIrq();
    ~NetHardIrq() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    std::vector<oeaware::Topic> subscribeTopics;
    const int slidingWinLen = 200; // for match que and thread info
    bool irqbalanceStatus = false; // irqbalance status before tune
    int netDataInterval = 0; // unit ms
    int numaNuma = 0;
    std::vector<int> cpu2Numa;
    bool envInit = false;
    std::vector<RecNetQueue> queueData;
    std::vector<RecNetThreads> threadData;
    void UpdateSystemInfo(const DataList &dataList);
    void UpdateNetInfo(const DataList &dataList);
    void UpdateQueueData(const PmuData *data, int dataLen);
    void UpdateThreadData(const PmuData *data, int dataLen);
    void MatchThreadAndQueue();
    void UpdateThreadAndQueueInfo(const RecNetQueue &queData, const RecNetThreads &thrData);
};
}

#endif // !HARDIRQ_TUNE_H

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
#include "oeaware/data/env_data.h"
#include "oeaware/data/net_hardirq_tune_data.h"
#include "oeaware/data/pmu_sampling_data.h"
#include "libkperf/pmu.h"
#include "irq_frontend.h"

namespace oeaware {
const int INVAILD_IRQ_ID = -1;
const int INVALID_CPU_ID = -1;
const int NET_RXT_THRESHOLD = 10; // < 10 means infrequent networks
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

struct QueueInfo {
    int queueId;
    int irqId = INVAILD_IRQ_ID;
    int rxSum = 0;
    int lastBindCpu = INVALID_CPU_ID;
    int rxMaxNode = 0;
    std::vector<int> numaRxTimes;
};

struct CpuSort {
    int cpu;
    float ratio;
    CpuSort(int c, float r) : cpu(c), ratio(r) {}
};

struct HardIrqMigUint {
    int irqId;
    int rxSum;
    int queId;
    std::string dev;
    int lastBindCore;
    int preferredNode;
    int priority;
    std::string GetInfo() const
    {
        return "dev " + dev + ", irq " + std::to_string(irqId) + ", queId  " + std::to_string(queId) +
            ", rxSum " + std::to_string(rxSum) + ", lastBindCore " + std::to_string(lastBindCore) +
            ", preferredNode " + std::to_string(preferredNode) + ", priority " + std::to_string(priority);
    }
};

struct ThreadNumaInfo {
    std::unordered_map<int, std::unordered_map<int, uint64_t>> queRxTimes; // ifIdx,que times
    std::vector<uint64_t> cyclesNumaRatio;  // numa distribution for approximating network threads
    uint64_t cyclesSum;
};
class NetHardIrq : public Interface {
public:
    NetHardIrq();
    ~NetHardIrq() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
    struct IrqInfo {
        std::string originAffinity;
        int lastAffinity;
        bool isTuned = false;
    };

private:
    bool highNoiseSample = false;
    std::vector<oeaware::Topic> subscribeTopics;
    std::unordered_map<std::string, std::unordered_map<int, QueueInfo>> netQueue;
    std::unordered_map<int, struct IrqInfo> irqInfo;
    const int slidingWinLen = 200; // for match que and thread info
    bool irqbalanceStatus = false; // irqbalance status before tune
    int netDataInterval = 0; // unit ms
    unsigned int numaNum = 0;
    std::vector<int> cpu2Numa;
    std::unordered_map<uint32_t, std::string> ifIdxToName;
    bool envInit = false;
    std::map<std::string, std::string> cmdHelp = {
        {"netdata", std::string("") +
         "    -netdata <type>        specify the source of network data, <type> should be : \n" +
         "                           thread_recv_que                use net_thread_que_data topic," +
         " low load collect, only support tcp(default)\n" +
         "                           skb_copy                       use skb copy, high load collect\n"},
        {"verbose",
         "    -verbose <on/off>      on:show verbose info, off(default):hide verbose info"},
    };
    std::vector<std::vector<uint64_t>> cpuTimeDiff;
    std::vector<std::vector<float>> cpuUtil;
    std::vector<RecNetQueue> queueData;
    std::vector<RecNetThreads> threadData;
    std::unordered_map<uint32_t, ThreadNumaInfo> threadNumaInfo;
    void Init();
    void InitIrqInfo();
    bool ResolveCmd(const std::string &cmd);
    std::string GetHelp();
    void UpdateSystemInfo(const DataList &dataList);
    void UpdatePmuSampleInfo(const DataList &dataList);
    void UpdateEnvInfo(const DataList &dataList);
    void UpdateNetIntfInfo(const DataList &dataList);
    void UpdateCpuInfo(const EnvCpuUtilParam *data);
    void UpdateQueueData(const PmuData *data, int dataLen);
    void UpdateThreadData(const PmuData *data, int dataLen);
    void UpdateCyclesData(const PmuData *data, int dataLen);
    void AddIrqToQueueInfo();
    void MatchThreadAndQueue();
    void MatchThreadQueAndNuma();
    void UpdateThreadAndQueueInfo(const RecNetQueue &queData, const RecNetThreads &thrData);
    void ClearInvalidQueueInfo();
    void CalCpuUtil();
    void TunePreprocessing();
    void MigrateHardIrq();
    void ResetCpuInfo();
    void ResetNetQueue();
    std::vector<std::vector<CpuSort>> SortNumaCpuUtil();
    void Tune();
    // read conf and resolve queue to irq
    IrqFrontEnd conf;
    std::unordered_map<std::string, EthQueInfo> ethQueData;
    // for debug
    bool showVerbose = false;
    bool debugTopicOpen = false;
    uint64_t runCnt = 0;
    std::string debugLog;
    template<typename F>
    void AddLog(F &&logFunc) // delay call
    {
        if (showVerbose || debugTopicOpen) {
            debugLog += logFunc();
        }
    }
    std::string CpuSortLog(const std::vector<std::vector<CpuSort>> &cpuSort);
    void PublishData();
};
}

#endif // !HARDIRQ_TUNE_H

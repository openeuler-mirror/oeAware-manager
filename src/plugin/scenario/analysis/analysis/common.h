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
#ifndef COMMON_H
#define COMMON_H

#include "pmu.h"
#include <vector>
#include <unordered_map>
#include "env.h"
#include <string>

struct NetworkInfo {
    void Init();
    uint64_t netRxSum = 0;
    std::vector<uint64_t> netRxTimes; // local net info
    void ClearData();
};

struct TaskInfo {
    TaskInfo();
    ~TaskInfo() = default;
    void Init();
    uint64_t loopCnt = 0; // note : loop count means the number of trace in the summary
    float numaScore = 0.0;
    uint64_t cycles = 0;
    uint64_t accessSum = 0;
    uint64_t accessCost = 0;
    std::vector<std::vector<uint64_t>> access;
    NetworkInfo netInfo; // only statistics system info temporarily
    void CalculateNumaScore();
    void ClearData();
};

struct Thread {
    int tid = -1;
    unsigned int cpu = 0;
    TaskInfo realtimeInfo;
    TaskInfo summaryInfo;
    std::vector<TaskInfo> traceInfo;
};

struct Proc {
    int pid = -1;
    std::string procName = "unknown";
    std::unordered_map<int, Thread> threads;
    std::vector<void *> pageVa;
    std::vector<const PmuData *> speBuff;
    TaskInfo realtimeInfo;
    TaskInfo summaryInfo;
    std::vector<TaskInfo> traceInfo;
    void SummaryThreads();
    void CalculateNumaScore();
    void ClearRealtimeInfo();
};

struct SystemInfo {
    std::unordered_map<int, Proc> procs;
    TaskInfo realtimeInfo;
    TaskInfo summaryInfo;
    std::vector<TaskInfo> traceInfo;
    void Init();
    void SummaryProcs();
    void CalculateNumaScore();
    void SetLoopCnt(uint64_t loopCnt);
    void ClearRealtimeInfo();
    void AppendTraceInfo();
    void TraceInfoSummary();
    void Reset();
};

#endif

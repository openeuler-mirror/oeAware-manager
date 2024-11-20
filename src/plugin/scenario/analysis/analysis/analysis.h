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
#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <iostream>
#include <unordered_map>
#include "common.h"
#include <vector>
#include "env.h"
#include <string>

struct RecNetQueue {
    uint64_t ts;
    const void *skbaddr;
    uint32_t core;
    std::string dev;
    uint64_t queueMapping;
};

struct RecNetThreads {
    uint64_t ts;
    uint32_t core;
    int pid;
    int tid;
    const void *skbaddr;
};

enum InstanceName {
    NUMA_TUNE,
    IRQ_TUNE,
    GAZELLE_TUNE,
    SMC_TUNE,
    STEALTASK_TUNE,
};

struct Instance {
    std::string name = "unknown";
    bool suggest = false;
    std::string notes;
};

class Analysis {
private:
    SystemInfo sysInfo;
    std::unordered_map<InstanceName, Instance> tuneInstances;
    std::vector<RecNetQueue> recNetQueue;
    std::vector<RecNetThreads> recNetThreads;
	std::vector<std::string> analysisData;
    uint64_t loopCnt = 0;
    void InstanceInit();
    void UpdateSpe(int dataLen, const PmuData *data);
    void UpdateAccess();
    void UpdateCyclesSample(int dataLen, const PmuData *data);
    void UpdateNetRx(int dataLen, const PmuData *data);
    void UpdateNapiGroRec(int dataLen, const PmuData *data);
    void UpdateSkbCopyDataIovec(int dataLen, const PmuData *data);
    void UpdateRemoteNetInfo(const RecNetQueue &queData, const RecNetThreads &threadData);
    void UpdateRecNetQueue();
    void NumaTuneSuggest(const TaskInfo &taskInfo, bool isSummary);
    void NetTuneSuggest(const TaskInfo &taskInfo, bool isSummary);
    void StealTaskTuneSuggest(const TaskInfo &taskInfo, bool isSummary);
    void Summary();
    void ShowNetInfoSummary();
    void ShowSummary();
	void BuildNetInfoSummary();
    void BuildSummary();
public:
    Env &env = Env::GetInstance();
    int GetPeriod();
    void Init();
    void UpdatePmu(const std::string &eventName, int dataLen, const PmuData *data);
    void Analyze();
	std::vector<std::string> GetAnalysisData();
    void Exit();
};

#endif
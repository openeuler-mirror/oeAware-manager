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

#include <algorithm>
#include <iostream>
#include <securec.h>
#include <dirent.h>
#include "pmu_uncore_collector.h"
#include "pmu_uncore_data.h"
#include "pmu_uncore.h"
#include "pcerrc.h"

PmuUncoreCollector::PmuUncoreCollector(): oeaware::Interface()
{
    this->name = "pmu_uncore_collector";
    this->version = "1.0.0";
    this->description = "collect uncore information of pmu";
    this->priority = 0;
    this->type = 0;
    this->period = 100;
    eventStr = { "rx_outer", "rx_sccl", "rx_ops_num" };
    pmuId = -1;
    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = topicStr;
    supportTopics.push_back(topic);
}

void PmuUncoreCollector::InitUncoreAttr(struct PmuAttr &attr)
{
    attr.pidList = nullptr;
    attr.numPid = 0;
    attr.cpuList = nullptr;
    attr.numCpu = 0;
    attr.evtAttr = nullptr;
    attr.period = 10;
    attr.useFreq = 0;
    attr.excludeUser = 0;
    attr.excludeKernel = 0;
    attr.symbolMode = NO_SYMBOL_RESOLVE;
    attr.callStack = 0;
    attr.dataFilter = SPE_FILTER_NONE;
    attr.evFilter = SPE_EVENT_NONE;
    attr.minLatency = 0;
    attr.includeNewFork = 0;

    DIR *dir = opendir("/sys/devices");
    if (dir != nullptr) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR && std::string(entry->d_name).find("hha") != std::string::npos) {
                hhaDir.push_back(std::string(entry->d_name) + "/");
            }
        }
        closedir(dir);
    }
}

int PmuUncoreCollector::OpenUncore(const oeaware::Topic &topic)
{
    struct PmuAttr attr;
    struct UncoreConfig *rxOuter;
    struct UncoreConfig *rxSccl;
    struct UncoreConfig *rxOpsNum;
    int hhaNum;
    int pd = -1;
    int ret;

    // Base on oeAware framework, uncore_open is called within uncore_enable.
    // If pmu_uncore is not supported, it will generate a large number of error logs.
    // So temporarily set uncore_is_open = true util oeAware framework provides open API.
    ret = HhaUncoreConfigInit();
    if (ret != 0) {
        printf("This system not support pmu_uncore\n");
        return pd;
    }

    hhaNum = GetUncoreHhaNum();
    rxOuter = GetRxOuter();
    rxSccl = GetRxSccl();
    rxOpsNum = GetRxOpsNum();

    char *evtList[hhaNum * UNCORE_MAX];
    for (int i = 0; i < hhaNum; i++) {
        evtList[i + hhaNum * RX_OUTER] = rxOuter[i].uncoreName;
        evtList[i + hhaNum * RX_SCCL] = rxSccl[i].uncoreName;
        evtList[i + hhaNum * RX_OPS_NUM] = rxOpsNum[i].uncoreName;
    }

    (void)memset_s(&attr, sizeof(struct PmuAttr), 0, sizeof(struct PmuAttr));

    attr.evtList = evtList;
    attr.numEvt = hhaNum * UNCORE_MAX;
    attr.pidList = NULL;
    attr.numPid = 0;
    attr.cpuList = NULL;
    attr.numCpu = 0;

    pd = PmuOpen(COUNTING, &attr);
    if (pd == -1) {
        printf("%s\n", Perror());
        return pd;
    }

    return pd;
}

oeaware::Result PmuUncoreCollector::OpenTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name || topic.topicName != topicStr) {
        return oeaware::Result(FAILED, "OpenTopic failed");
    }

    if (pmuId == -1) {
        pmuId = OpenUncore(topic);
        if (pmuId == -1) {
            return oeaware::Result(FAILED, "OpenTopic failed, PmuOpen failed");
        }
        PmuEnable(pmuId);
        timestamp = std::chrono::high_resolution_clock::now();
        return oeaware::Result(OK);
    }

    return oeaware::Result(FAILED);
}

void PmuUncoreCollector::CloseTopic(const oeaware::Topic &topic)
{
    if (pmuId == -1) {
        std::cout << "CloseTopic failed" << std::endl;
    } else {
        PmuDisable(pmuId);
        PmuClose(pmuId);
        pmuId = -1;
    }
}

oeaware::Result PmuUncoreCollector::Enable(const std::string &parma)
{
    return oeaware::Result(OK);
}

void PmuUncoreCollector::Disable()
{
    return;
}

void PmuUncoreCollector::UpdateData(const DataList &dataList)
{
    return;
}

void PmuUncoreCollector::Run()
{
    if (pmuId != -1) {
        PmuUncoreData *data = new PmuUncoreData();
        PmuDisable(pmuId);
        data->len = PmuRead(pmuId, &(data->pmuData));
        PmuEnable(pmuId);

        auto now = std::chrono::high_resolution_clock::now();
        data->interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp).count();
        timestamp = now;

        DataList dataList;
        dataList.topic.instanceName = "pmu_uncore_collector";
        dataList.topic.topicName = new char[topicStr.size() + 1];
        strcpy_s(dataList.topic.topicName, topicStr.size() + 1, topicStr.data());
        dataList.topic.params = "";
        dataList.len = 1;
        dataList.data = new void* [1];
        dataList.data[0] = data;
        Publish(dataList);
    }
}
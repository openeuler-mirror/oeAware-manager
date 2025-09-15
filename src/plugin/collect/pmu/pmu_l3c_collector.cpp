/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include "pmu_l3c_collector.h"
#include <algorithm>
#include <iostream>
#include <securec.h>
#include <dirent.h>
#include "oeaware/data/pmu_l3c_data.h"
#include "pmu_uncore.h"
#include "libkperf/pcerrc.h"

PmuL3cCollector::PmuL3cCollector(): oeaware::Interface()
{
    this->name = OE_PMU_L3C_COLLECTOR;
    this->version = "1.0.0";
    this->description = "collect l3c information of pmu";
    this->priority = 0;
    this->type = 0;
    this->period = 1000;
    eventStr = { "l3c_hit" };
    pmuId = -1;
    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = topicStr;
    topic.params = "";
    supportTopics.push_back(topic);
}

int PmuL3cCollector::OpenL3c()
{
    struct PmuAttr attr = {};
    struct UncoreConfig *l3cHit;
    int l3cNum;
    int pd = -1;
    int ret;

    // Base on oeAware framework, l3c_open is called within l3c_enable.
    // If pmu_l3c is not supported, it will generate a large number of error logs.
    // So temporarily set l3c_is_open = true util oeAware framework provides open API.
    ret = L3cUncoreConfigInit();
    if (ret != 0) {
        WARN(logger, "This system not support pmu_l3c");
        return pd;
    }

    l3cNum = GetUncoreL3cNum();
    l3cHit = GetL3cHit();

    char *evtList[l3cNum];
    for (int i = 0; i < l3cNum; i++) {
        evtList[i] = l3cHit[i].uncoreName;
    }

    attr.evtList = evtList;
    attr.numEvt = l3cNum;
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

oeaware::Result PmuL3cCollector::OpenTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name || topic.topicName != topicStr || topic.params != "") {
        return oeaware::Result(FAILED, "OpenTopic: " + topic.GetType() + " failed, topic not match");
    }

    if (pmuId == -1) {
        pmuId = OpenL3c();
        if (pmuId == -1) {
            return oeaware::Result(FAILED, "OpenTopic: " + topic.GetType() + " failed, PmuOpen failed");
        }
        PmuEnable(pmuId);
        timestamp = std::chrono::high_resolution_clock::now();
        return oeaware::Result(OK);
    }

    return oeaware::Result(FAILED);
}

void PmuL3cCollector::CloseTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name || topic.topicName != topicStr || topic.params != "") {
        WARN(logger, "CloseTopic:" + topic.GetType() + " failed, topic not match.");
        return;
    }
    if (pmuId == -1) {
        WARN(logger, "CloseTopic:" + topic.GetType() + " failed, pmuId = -1");
        return;
    }
    PmuDisable(pmuId);
    PmuClose(pmuId);
    UncoreConfigFini();
    pmuId = -1;
}

oeaware::Result PmuL3cCollector::Enable(const std::string &param)
{
    (void)param;
    if (!oeaware::FileExist(l3cPath)) {
        return oeaware::Result(FAILED, "the system does not support l3c events.");
    }
    return oeaware::Result(OK);
}

void PmuL3cCollector::Disable()
{
    return;
}

void PmuL3cCollector::UpdateData(const DataList &dataList)
{
    (void)dataList;
    return;
}

void PmuL3cCollector::Run()
{
    if (pmuId == -1) {
        return;
    }
    int l3cNum = GetUncoreL3cNum();
    PmuL3cData *l3cData = new PmuL3cData();
    PmuDisable(pmuId);
    l3cData->len = PmuRead(pmuId, &(l3cData->pmuData));
    if (l3cNum != l3cData->len) {
        WARN(logger, "PmuL3cCollector collect data length error.");
        return;
    }
    PmuEnable(pmuId);

    auto now = std::chrono::high_resolution_clock::now();
    l3cData->interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp).count();
    timestamp = now;

    DataList dataList;
    dataList.topic.instanceName = new char[name.size() + 1];
    strcpy_s(dataList.topic.instanceName, name.size() + 1, name.data());
    dataList.topic.topicName = new char[topicStr.size() + 1];
    strcpy_s(dataList.topic.topicName, topicStr.size() + 1, topicStr.data());
    dataList.topic.params = new char[1];
    dataList.topic.params[0] = 0;
    dataList.len = 1;
    dataList.data = new void *[1];
    dataList.data[0] = l3cData;
    Publish(dataList);
}
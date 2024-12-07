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
#include "pmu_spe_collector.h"
#include <algorithm>
#include <securec.h>
#include "oeaware/data/pmu_spe_data.h"

PmuSpeCollector::PmuSpeCollector(): oeaware::Interface()
{
    this->name = OE_PMU_SPE_COLLECTOR;
    this->version = "1.0.0";
    this->description = "collect spe information of pmu";
    this->priority = 0;
    this->type = 0;
    this->period = 100;
    pmuId = -1;
    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = topicStr;
    topic.params = "";
    supportTopics.push_back(topic);
}

void PmuSpeCollector::InitSpeAttr(struct PmuAttr &attr)
{
    attr.evtList = nullptr;
    attr.numEvt = 0;
    attr.pidList = nullptr;
    attr.numPid = 0;
    attr.cpuList = nullptr;
    attr.numCpu = 0;
    attr.evtAttr = nullptr;
    attr.period = attrPeriod;
    attr.useFreq = 0;
    attr.excludeUser = 0;
    attr.excludeKernel = 0;
    attr.symbolMode = NO_SYMBOL_RESOLVE;
    attr.callStack = 0;
    attr.dataFilter = SPE_DATA_ALL;
    attr.evFilter = SPE_EVENT_RETIRED;
    attr.minLatency = 0x60;
    attr.includeNewFork = 0;
}

int PmuSpeCollector::OpenSpe()
{
    struct PmuAttr attr;
    InitSpeAttr(attr);
    int pd = PmuOpen(SPE_SAMPLING, &attr);
    if (pd == -1) {
        WARN(logger, "open spe failed.");
    }

    return pd;
}

void PmuSpeCollector::DynamicAdjustPeriod(int interval)
{
    if (pmuId == -1) {
        return;
    }
    if (interval > timeout) {
        PmuDisable(pmuId);
        PmuClose(pmuId);
        attrPeriod *= periodThreshold;
        if (attrPeriod > maxAttrPeriod) {
            attrPeriod = maxAttrPeriod;
        }
        INFO(logger, "PmuSpeCollector dynamic adjust period to " <<
            attrPeriod << ", PmuRead interval is " << interval << " ms.");
        pmuId = OpenSpe();
        if (pmuId != -1) {
            PmuEnable(pmuId);
        }
    }
    // later add code to decrease period when interval is too small
}

oeaware::Result PmuSpeCollector::OpenTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name || topic.topicName != topicStr || topic.params != "") {
        return oeaware::Result(FAILED, "OpenTopic: " + topic.GetType() + " failed, topic not match");
    }
    attrPeriod = minAttrPeriod;
    readTimeMs = 0;
    if (pmuId == -1) {
        pmuId = OpenSpe();
        if (pmuId == -1) {
            return oeaware::Result(FAILED, "OpenTopic failed, PmuOpen failed");
        }
        PmuEnable(pmuId);
        timestamp = std::chrono::high_resolution_clock::now();
        return oeaware::Result(OK);
    }

    return oeaware::Result(FAILED);
}

void PmuSpeCollector::CloseTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name || topic.topicName != topicStr || topic.params != "") {
        WARN(logger, "CloseTopic:" + topic.GetType() + " failed, topic not match");
        return;
    }
    if (pmuId == -1) {
        WARN(logger, "CloseTopic:" + topic.GetType() + " failed, pmuId = -1");
        return;
    }
    PmuDisable(pmuId);
    PmuClose(pmuId);
    pmuId = -1;
}

oeaware::Result PmuSpeCollector::Enable(const std::string &param)
{
    (void)param;
    return oeaware::Result(OK);
}

void PmuSpeCollector::Disable()
{
    return;
}

void PmuSpeCollector::UpdateData(const DataList &dataList)
{
    (void)dataList;
    return;
}

void PmuSpeCollector::Run()
{
    // adjust period will pmuclose and free spe data
    // so adjust period should be done after other plugins have finished using SPE data
    DynamicAdjustPeriod(readTimeMs);
    if (pmuId == -1) {
        return;
    }
    PmuSpeData *data = new PmuSpeData();
    PmuDisable(pmuId);
    auto readBegin = std::chrono::high_resolution_clock::now();
    data->len = PmuRead(pmuId, &(data->pmuData));
    readTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - readBegin).count();
    PmuEnable(pmuId);
    auto now = std::chrono::high_resolution_clock::now();
    data->interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp).count();
    timestamp = std::chrono::high_resolution_clock::now();
    DataList dataList;
    dataList.topic.instanceName = new char[name.size() + 1];
    strcpy_s(dataList.topic.instanceName, name.size() + 1, name.data());
    dataList.topic.topicName = new char[topicStr.size() + 1];
    strcpy_s(dataList.topic.topicName, topicStr.size() + 1, topicStr.data());
    dataList.topic.params = new char[1];
    dataList.topic.params[0] = 0;
    dataList.len = 1;
    dataList.data = new void *[1];
    dataList.data[0] = data;
    Publish(dataList);
}
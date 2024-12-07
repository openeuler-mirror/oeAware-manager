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
#include "pmu_sampling_collector.h"
#include <algorithm>
#include <iostream>
#include <securec.h>
#include "oeaware/data/pmu_sampling_data.h"

PmuSamplingCollector::PmuSamplingCollector(): oeaware::Interface()
{
    this->name = OE_PMU_SAMPLING_COLLECTOR;
    this->version = "1.0.0";
    this->description = "collect sampling information of pmu";
    this->priority = 0;
    this->type = 0;
    this->period = 100;
    for (const auto &it : topicStr) {
        oeaware::Topic topic;
        topic.instanceName = this->name;
        topic.topicName = it;
        topic.params = "";
        supportTopics.push_back(topic);
    }
}

void PmuSamplingCollector::InitSamplingAttr(struct PmuAttr &attr)
{
    attr.pidList = nullptr;
    attr.numPid = 0;
    attr.cpuList = nullptr;
    attr.numCpu = 0;
    attr.evtAttr = nullptr;
    attr.excludeUser = 0;
    attr.excludeKernel = 0;
    attr.symbolMode = NO_SYMBOL_RESOLVE;
    attr.callStack = 0;
    attr.dataFilter = SPE_FILTER_NONE;
    attr.evFilter = SPE_EVENT_NONE;
    attr.minLatency = 0;
    attr.includeNewFork = 0;
}

int PmuSamplingCollector::OpenSampling(const oeaware::Topic &topic)
{
    struct PmuAttr attr;
    InitSamplingAttr(attr);

    char *evtList[1];
    evtList[0] = new char[topic.topicName.length() + 1];
    errno_t ret = strcpy_s(evtList[0], topic.topicName.length() + 1, topic.topicName.c_str());
    if (ret != EOK) {
        std::cout << topic.topicName << " open failed, reason: strcpy_s failed" << std::endl;
        return -1;
    }
    attr.evtList = evtList;
    attr.numEvt = 1;
    if (topic.topicName == "cycles") {
        attr.freq = CYCLES_FREQ;
        attr.useFreq = 1;
        attr.symbolMode = RESOLVE_ELF;
    } else {
        attr.period = NET_RECEIVE_TRACE_SAMPLE_PERIOD;
    }
    int pd = PmuOpen(SAMPLING, &attr);
    if (pd == -1) {
        std::cout << topic.topicName << " open failed" << std::endl;
    }

    return pd;
}

oeaware::Result PmuSamplingCollector::OpenTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name) {
        return oeaware::Result(FAILED, "OpenTopic:" + topic.GetType() + " failed, instanceName is not match");
    }

    if (topic.params != "") {
        return oeaware::Result(FAILED, "OpenTopic:" + topic.GetType() + " failed, params is not empty");
    }

    for (auto &iter : topicStr) {
        if (iter != topic.topicName) {
            continue;
        }
        if (topicParams[iter].open) {
            WARN(logger, topic.GetType() << " has been opened before!");
            return oeaware::Result(OK);
        }
        topicParams[iter].pmuId = OpenSampling(topic);
        if (topicParams[iter].pmuId == -1) {
            return oeaware::Result(FAILED, "OpenTopic failed, PmuOpen failed");
        }
        PmuEnable(topicParams[iter].pmuId);
        topicParams[iter].timestamp = std::chrono::high_resolution_clock::now();
        topicParams[iter].open = true;
        return oeaware::Result(OK);
    }
    return oeaware::Result(FAILED, "OpenTopic " + topic.GetType() + "failed, no support topic!");
}

void PmuSamplingCollector::CloseTopic(const oeaware::Topic &topic)
{
    if (!topicParams[topic.topicName].open) {
        WARN(logger, "CloseTopic failed, " + topic.GetType() + " not open.");
        return;
    }
    PmuDisable(topicParams[topic.topicName].pmuId);
    PmuClose(topicParams[topic.topicName].pmuId);
    topicParams[topic.topicName].pmuId = -1;
    topicParams[topic.topicName].open = false;
}

oeaware::Result PmuSamplingCollector::Enable(const std::string &param)
{
    (void)param;
    return oeaware::Result(OK);
}

void PmuSamplingCollector::Disable()
{
    return;
}

void PmuSamplingCollector::UpdateData(const DataList &dataList)
{
    (void)dataList;
    return;
}

void PmuSamplingCollector::Run()
{
    for (auto &topicName : topicStr) {
        if (!topicParams[topicName].open) {
            continue;
        }
        int pmuId = topicParams[topicName].pmuId;
        PmuSamplingData *data = new PmuSamplingData();
        PmuDisable(pmuId);
        data->len = PmuRead(pmuId, &(data->pmuData));
        PmuEnable(pmuId);
        // calculate the actual collection time
        auto now = std::chrono::high_resolution_clock::now();
        data->interval = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - topicParams[topicName].timestamp).count();
        topicParams[topicName].timestamp = now;
        DataList dataList;
        dataList.topic.instanceName = new char[name.size() + 1];
        strcpy_s(dataList.topic.instanceName, name.size() + 1, name.data());
        dataList.topic.topicName = new char[topicName.size() + 1];
        strcpy_s(dataList.topic.topicName, topicName.size() + 1, topicName.data());
        dataList.topic.params = new char[1];
        dataList.topic.params[0] = 0;
        dataList.data = new void *[1];
        dataList.len = 1;
        dataList.data[0] = data;
        Publish(dataList);
    }
}

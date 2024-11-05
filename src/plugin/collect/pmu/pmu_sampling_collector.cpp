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
#include "pmu_sampling_collector.h"
#include "pmu_sampling_data.h"

PmuSamplingCollector::PmuSamplingCollector(): oeaware::Interface()
{
    this->name = "pmu_sampling_collector";
    this->version = "1.0.0";
    this->description = "collect sampling information of pmu";
    this->priority = 0;
    this->type = 0;
    this->period = 100;
    for (const auto &it : topicStr) {
        pmuId[it] = -1;
        oeaware::Topic topic;
        topic.instanceName = this->name;
        topic.topicName = it;
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
    attr.freq = 100;
    attr.useFreq = 1;
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

    int pd = PmuOpen(SAMPLING, &attr);
    if (pd == -1) {
        std::cout << topic.topicName << " open failed" << std::endl;
    }

    return pd;
}

oeaware::Result PmuSamplingCollector::OpenTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name) {
        return oeaware::Result(FAILED, "OpenTopic failed");
    }

    for (auto &iter : topicStr) {
        if (iter == topic.topicName) {
            if (pmuId[iter] == -1) {
                pmuId[iter] = OpenSampling(topic);
                if (pmuId[iter] == -1) {
                    return oeaware::Result(FAILED, "OpenTopic failed, PmuOpen failed");
                }
                PmuEnable(pmuId[iter]);
                timestamp = std::chrono::high_resolution_clock::now();
                return oeaware::Result(OK);
            }
        }
    }

    return oeaware::Result(FAILED);
}

void PmuSamplingCollector::CloseTopic(const oeaware::Topic &topic)
{
    if (pmuId[topic.topicName] == -1) {
        std::cout << "CloseTopic failed" << std::endl;
    } else {
        PmuDisable(pmuId[topic.topicName]);
        PmuClose(pmuId[topic.topicName]);
        pmuId[topic.topicName] = -1;
    }
}

oeaware::Result PmuSamplingCollector::Enable(const std::string &parma)
{
    return oeaware::Result(OK);
}

void PmuSamplingCollector::Disable()
{
    return;
}

void PmuSamplingCollector::UpdateData(const DataList &dataList)
{
    return;
}

void PmuSamplingCollector::Run()
{
    for (auto &iter : pmuId) {
        if (iter.second != -1) {
            PmuSamplingData *data = new PmuSamplingData();
            PmuDisable(iter.second);
            data->len = PmuRead(iter.second, &(data->pmuData));
            PmuEnable(iter.second);

            auto now = std::chrono::high_resolution_clock::now();
            data->interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp).count();
            timestamp = now;

            DataList dataList;
            dataList.topic.instanceName = new char[name.size() + 1];
            strcpy_s(dataList.topic.instanceName, name.size() + 1, name.data());
            dataList.topic.topicName = new char[iter.first.size() + 1];
            strcpy_s(dataList.topic.topicName, iter.first.size() + 1, iter.first.data());
            dataList.topic.params = new char[1];
            dataList.topic.params[0] = 0;
            dataList.data = new void* [1];
            dataList.len = 1;
            dataList.data[0] = data;
            Publish(dataList);
        }
    }
}

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
#include "pmu_spe_collector.h"
#include "pmu_spe_data.h"

PmuSpeCollector::PmuSpeCollector(): oeaware::Interface()
{
    this->name = "pmu_spe_collector";
    this->version = "1.0.0";
    this->description = "collect spe information of pmu";
    this->priority = 0;
    this->type = 0;
    this->period = 100;
    pmuId = -1;
    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = topicStr;
    supportTopics.push_back(topic);
}

void PmuSpeCollector::InitSpeAttr(struct PmuAttr &attr)
{
    attr.pidList = nullptr;
    attr.numPid = 0;
    attr.cpuList = nullptr;
    attr.numCpu = 0;
    attr.evtAttr = nullptr;
    attr.period = 2048;
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

int PmuSpeCollector::OpenSpe(const oeaware::Topic &topic)
{
    struct PmuAttr attr;
    InitSpeAttr(attr);

    char *evtList[1];
    evtList[0] = new char[topic.topicName.length() + 1];
    errno_t ret = strcpy_s(evtList[0], topic.topicName.length() + 1, topic.topicName.c_str());
    if (ret != EOK) {
        std::cout << topic.topicName << " open failed, reason: strcpy_s failed" << std::endl;
        return -1;
    }
    attr.evtList = evtList;
    attr.numEvt = 1;

    int pd = PmuOpen(SPE_SAMPLING, &attr);
    if (pd == -1) {
        std::cout << "open spe failed" << std::endl;
    }

    return pd;
}

int PmuSpeCollector::OpenTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name || topic.topicName != topicStr) {
        std::cout << "OpenTopic failed" << std::endl;
        return -1;
    }

    if (pmuId == -1) {
        pmuId = OpenSpe(topic);
        if (pmuId == -1) {
            std::cout << "OpenTopic failed, PmuOpen failed" << std::endl;
            return -1;
        }
        PmuEnable(pmuId);
        timestamp = std::chrono::high_resolution_clock::now();
        return 0;
    }

    return -1;
}

void PmuSpeCollector::CloseTopic(const oeaware::Topic &topic)
{
    if (pmuId == -1) {
        std::cout << "CloseTopic failed" << std::endl;
    } else {
        PmuDisable(pmuId);
        PmuClose(pmuId);
        pmuId = -1;
    }
}

int PmuSpeCollector::Enable(const std::string &parma)
{
    return 0;
}

void PmuSpeCollector::Disable()
{
    return;
}

void PmuSpeCollector::UpdateData(const oeaware::DataList &dataList)
{
    return;
}

void PmuSpeCollector::Run()
{
    if (pmuId != -1) {
        std::shared_ptr <PmuSpeData> data = std::make_shared<PmuSpeData>();
        PmuDisable(pmuId);
        data->len = PmuRead(pmuId, &(data->pmuData));
        PmuEnable(pmuId);

        auto now = std::chrono::high_resolution_clock::now();
        data->interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp).count();
        timestamp = now;

        struct oeaware::DataList dataList;
        dataList.topic.instanceName = this->name;
        dataList.topic.topicName = topicStr;
        dataList.data.push_back(data);
        Publish(dataList);
    }
}

void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<PmuSpeCollector>());
}
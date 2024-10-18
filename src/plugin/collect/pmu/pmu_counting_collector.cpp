/*
 * Copyright (c) 2024-2024 Huawei Technologies Co., Ltd. All rights reserved.
 *
 * numafast is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include <algorithm>
#include <iostream>
#include <securec.h>
#include "pmu_counting_collector.h"
#include "pmu_counting_data.h"

PmuCountingCollector::PmuCountingCollector(): oeaware::Interface()
{
    this->name = "pmu_counting_collector";
    this->version = "1.0.0";
    this->description = "collect counting information of pmu";
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

void PmuCountingCollector::InitPmuAttr(struct PmuAttr &attr)
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
}

int PmuCountingCollector::OpenCounting(const oeaware::Topic &topic)
{
    struct PmuAttr attr;
    InitPmuAttr(attr);

    char *evtList[1];
    evtList[0] = new char[topic.topicName.length() + 1];
    errno_t ret = strcpy_s(evtList[0], topic.topicName.length() + 1, topic.topicName.c_str());
    if (ret != EOK) {
        std::cout << topic.topicName << " open failed, reason: strcpy_s failed" << std::endl;
        return -1;
    }
    attr.evtList = evtList;
    attr.numEvt = 1;

    int pd = PmuOpen(COUNTING, &attr);
    if (pd == -1) {
        std::cout << topic.topicName << " open failed" << std::endl;
    }

    return pd;
}

int PmuCountingCollector::OpenTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name) {
        std::cout << "OpenTopic failed" << std::endl;
        return -1;
    }

    for (auto &iter : topicStr) {
        if (iter == topic.topicName) {
            if (pmuId[iter] == -1) {
                pmuId[iter] = OpenCounting(topic);
                if (pmuId[iter] == -1) {
                    std::cout << "OpenTopic failed, PmuOpen failed" << std::endl;
                    return -1;
                }
                PmuEnable(pmuId[iter]);
                return 0;
            }
        }
    }

    return -1;
}

void PmuCountingCollector::CloseTopic(const oeaware::Topic &topic)
{
    if (pmuId[topic.topicName] == -1) {
        std::cout << "CloseTopic failed" << std::endl;
    } else {
        PmuDisable(pmuId[topic.topicName]);
        PmuClose(pmuId[topic.topicName]);
        pmuId[topic.topicName] = -1;
    }
}

int PmuCountingCollector::Enable()
{
    return 0;
}

void PmuCountingCollector::Disable()
{
    return;
}

void PmuCountingCollector::UpdateData(const oeaware::DataList &dataList)
{
    return;
}

void PmuCountingCollector::Run()
{
    for (auto &iter : pmuId) {
        if (iter.second != -1) {
            std::shared_ptr <PmuCountingData> data = std::make_shared<PmuCountingData>();
            PmuDisable(iter.second);
            data->len = PmuRead(iter.second, &(data->pmuData));
            PmuEnable(iter.second);

            struct oeaware::DataList dataList;
            dataList.topic.instanceName = this->name;
            dataList.topic.topicName = iter.first;
            dataList.data.push_back(data);
            Publish(dataList);
        }
    }
}

void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<PmuCountingCollector>());
}
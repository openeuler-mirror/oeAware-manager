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
    InitUncoreAttr(attr);

    if (hhaDir.size() == 0) {
        std::cout << "uncore hha devices don't exist." << std::endl;
        return -1;
    }

    int evt_num = hhaDir.size() * eventStr.size();
    char **evtList = new char *[evt_num];
    for (size_t i = 0; i < hhaDir.size(); ++i) {
        for (size_t j = 0; j < eventStr.size(); ++j) {
            int evt_len = hhaDir[i].size() + eventStr[j].size() + 2; // +2 for '/' and '\0'
            evtList[i * eventStr.size() + j] = new char[evt_len];
            errno_t ret = snprintf_s(evtList[i * eventStr.size() + j], evt_len, evt_len - 1, "%s%s/",
                                     hhaDir[i].c_str(), eventStr[j].c_str());
            if (ret != EOK) {
                std::cout << "OpenTopic failed, reason: snprintf_s failed" << std::endl;
                return -1;
            }
        }
    }

    attr.evtList = evtList;
    attr.numEvt = evt_num;
    int pd = PmuOpen(COUNTING, &attr);

    for (int i = 0; i < evt_num; ++i) {
        delete[] evtList[i];
    }
    delete[] evtList;

    if (pd == -1) {
        std::cout << topic.topicName << " open failed" << std::endl;
    }

    return pd;
}

int PmuUncoreCollector::OpenTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name || topic.topicName != topicStr) {
        std::cout << "OpenTopic failed" << std::endl;
        return -1;
    }

    if (pmuId == -1) {
        pmuId = OpenUncore(topic);
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

int PmuUncoreCollector::Enable(const std::string &parma)
{
    return 0;
}

void PmuUncoreCollector::Disable()
{
    return;
}

void PmuUncoreCollector::UpdateData(const oeaware::DataList &dataList)
{
    return;
}

void PmuUncoreCollector::Run()
{
    if (pmuId != -1) {
        std::shared_ptr <PmuUncoreData> data = std::make_shared<PmuUncoreData>();
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
    interface.emplace_back(std::make_shared<PmuUncoreCollector>());
}
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
#include "thread_aware.h"
#include <securec.h>

namespace oeaware {
ThreadAware::ThreadAware()
{
    name = "thread_scenario";
    period = awarePeriod;
    priority = 1;
    type = SCENARIO;
    supportTopics.emplace_back(Topic{"thread_scenario", "thread_scenario", ""});
}

bool ThreadAware::ReadKeyList(const std::string &fileName)
{
    std::ifstream file(fileName);
    if (!file.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        keyList.emplace_back(line);
    }
    file.close();
    return true;
}
Result ThreadAware::OpenTopic(const Topic &topic)
{
    (void)topic;
    return Result(OK);
}

void ThreadAware::CloseTopic(const Topic &topic)
{
    (void)topic;
}

Result ThreadAware::Enable(const std::string &param)
{
    (void)param;
    if (!ReadKeyList(configPath)) {
        return Result(FAILED);
    }
    Subscribe(Topic{"thread_collector", "thread_collector", ""});
    return Result(OK);
}

void ThreadAware::Disable()
{
    Unsubscribe(Topic{"thread_collector", "thread_collector", ""});
    keyList.clear();
    threadWhite.clear();
}

void ThreadAware::UpdateData(const DataList &dataList)
{
    tmpData.clear();
    for (uint64_t i = 0; i < dataList.len; ++i) {
        ThreadInfo *data = (ThreadInfo *)dataList.data[i];
        tmpData.emplace_back(*data);
    }
}

void ThreadAware::Run()
{
    for (auto &threadInfo : tmpData) {
        for (size_t j = 0; j < keyList.size(); ++j) {
            if (threadInfo.name == keyList[j]) {
                threadWhite.emplace_back(threadInfo);
                break;
            }
        }
    }
    DataList dataList;
    dataList.topic.instanceName = new char[name.size() + 1];
    strcpy_s(dataList.topic.instanceName, name.size() + 1, name.data());
    dataList.topic.topicName = new char[name.size() + 1];
    strcpy_s(dataList.topic.topicName, name.size() + 1, name.data());
    dataList.topic.params = new char[1];
    dataList.topic.params[0] = 0;
    dataList.len = threadWhite.size();
    dataList.data = new void* [dataList.len];
    for (uint64_t i = 0; i < dataList.len; ++i) {
        dataList.data[i] = &threadWhite[i];
    }
    Publish(dataList);
    threadWhite.clear();
}

extern "C" void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<ThreadAware>());
}
}

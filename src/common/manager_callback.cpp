/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include "manager_callback.h"

namespace oeaware {
void ManagerCallback::Init(EventQueue newRecvData)
{
    recvData = newRecvData;
}

int ManagerCallback::Subscribe(const std::string &name, const Topic &topic, int type)
{
    auto topicName = topic.GetType();
    std::lock_guard<std::mutex> lock(mutex);
    if (type) {
        topicInstance[topicName].insert(name);
    } else {
        topicSdk[topicName].insert(name);
    }
    inDegree[topic.instanceName][topic.topicName][topic.params]++;
    return 0;
}

int ManagerCallback::Unsubscribe(const std::string &name, const Topic &topic, int type)
{
    auto topicName = topic.GetType();
    std::lock_guard<std::mutex> lock(mutex);
    if (type) {
        topicInstance[topicName].erase(name);
    } else {
        topicSdk[topicName].erase(name);
    }
    --inDegree[topic.instanceName][topic.topicName][topic.params];
    return 0;
}

std::vector<std::string> ManagerCallback::Unsubscribe(const std::string &name)
{
    std::vector<std::string> ret;
    std::lock_guard<std::mutex> lock(mutex);
    for (auto &p : topicSdk) {
        auto &subscriber = p.second;
        auto topicType = p.first;
        auto names = SplitString(topicType, "::");
        auto instanceName = names[0];
        auto topicName = names[1];
        auto params = (names.size() > 2 ? names[2] : "");
        if (subscriber.count(name)) {
            subscriber.erase(name);
            ret.emplace_back(instanceName);
            --inDegree[instanceName][topicName][params];
        }
    }
    return ret;
}

void ManagerCallback::Publish(const DataList &dataList)
{
    auto &topic = dataList.topic;
    auto topicName = topic.GetType();
    for (auto &name : topicSdk[topicName]) {
        // sdk, send message to romte
        OutStream out;
        dataList.Serialize(out);
        recvData->Push(Event(Opt::DATA, {name, out.Str()}));
    }
    publishData.emplace_back(dataList);
}
}

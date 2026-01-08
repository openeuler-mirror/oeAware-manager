/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include "soft_domain_tune.h"

using namespace oeaware;

SoftDomainTune::SoftDomainTune()
{
    name = "soft_domain_tune";
    description = "Enable SOFT_DOMAIN scheduling feature";
    version = "1.0.0";
    period = defaultPeriod;
    priority = defaultPriority;
    type = TUNE;
    
    // 订阅docker采集数据
    Topic topic;
    topic.instanceName = OE_DOCKER_COLLECTOR;
    topic.topicName = OE_DOCKER_COLLECTOR;
    subscribeTopics.push_back(topic);
}

oeaware::Result SoftDomainTune::OpenTopic(const Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void SoftDomainTune::CloseTopic(const Topic &topic)
{
    (void)topic;
}

void SoftDomainTune::UpdateData(const DataList &dataList)
{
    Topic topic{dataList.topic.instanceName, dataList.topic.topicName, dataList.topic.params};
    if (topic.instanceName == OE_DOCKER_COLLECTOR && topic.topicName == OE_DOCKER_COLLECTOR) {
        UpdateDockerData(dataList);
    } else {
        WARN(logger, "Unknown topic, {instanceName:" + topic.instanceName + ", topicName:" + topic.topicName + "}.");
    }
}

oeaware::Result SoftDomainTune::Enable(const std::string &param)
{
    (void)param;
    
    // 订阅docker采集数据
    for (auto &topic : subscribeTopics) {
        Result ret = Subscribe(topic);
        if (ret.code != OK) {
            ERROR(logger, "Failed to subscribe topic: " << topic.instanceName << "/" << topic.topicName);
            return ret;
        }
    }
    
    // TODO: 实现具体功能
    return oeaware::Result(OK, "Soft domain tune enabled");
}

void SoftDomainTune::Disable()
{
    // 取消订阅
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    
    // 清空docker信息
    dockerContainers.clear();
    
    // TODO: 实现具体功能
}

void SoftDomainTune::Run()
{
    // 每个周期都会调用Run()，此时dockerContainers已经通过UpdateData更新
    // TODO: 实现具体功能，可以使用dockerContainers中的数据
}

void SoftDomainTune::UpdateDockerData(const DataList &dataList)
{
    // 使用unordered_set记录当前周期收到的docker ID
    std::unordered_set<std::string> currentDockerIds;
    
    // 更新docker信息
    for (uint64_t i = 0; i < dataList.len; i++) {
        auto *container = static_cast<Container*>(dataList.data[i]);
        if (container == nullptr) {
            continue;
        }
        
        // 存储或更新docker信息
        dockerContainers[container->id] = *container;
        currentDockerIds.insert(container->id);
    }
    
    // 移除已经不存在的docker（如果某个docker在当前数据中不存在，说明它已经被删除）
    for (auto it = dockerContainers.begin(); it != dockerContainers.end();) {
        if (currentDockerIds.find(it->first) == currentDockerIds.end()) {
            // docker已不存在，从map中移除
            it = dockerContainers.erase(it);
        } else {
            ++it;
        }
    }
    
    DEBUG(logger, "Updated docker data, current docker count: " << dockerContainers.size());
}


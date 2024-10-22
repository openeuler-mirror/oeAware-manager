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
#include "subscribe_handler.h"

namespace oeaware {
Result SubscribeHandler::Subscribe(const std::string &name, const Topic &topic)
{
    Result result;
    if (!memoryStore->IsInstanceExist(topic.topicName)) {
        WARN(logger, "The subscribed instance " << topic.instanceName << " does not exist.");
        result.code = -1;
        return result;
    }
    auto instance = memoryStore->GetInstance(topic.instanceName);
    if (!instance->supportTopics.count(topic.topicName)) {
        WARN(logger, "The subscribed topic " << topic.topicName << " does not exist.");
        result.code = -1;
        return result;
    }
    if (managerCallback->Subscribe(name, topic, 0) < 0) {
        WARN(logger, "The subscribed topic " << topic.topicName << " failed.");
        result.code = -1;
        return result;
    }
    result.code = 0;
    return result;
}

EventResult SubscribeHandler::Handle(const Event &event)
{
    Topic topic;
    InStream in(event.payload[0]);
    topic.Deserialize(in);
    EventResult eventResult;
    Result result = Subscribe(event.payload[1], topic);
    eventResult.opt = Opt::SUBSCRIBE;
    eventResult.payload.emplace_back(encode(result));
    return eventResult;
}
}
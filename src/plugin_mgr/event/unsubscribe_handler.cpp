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
#include "unsubscribe_handler.h"

namespace oeaware {
EventResult UnsubscribeHandler::Handle(const Event &event)
{
    EventResult eventResult;
    Result result;
    if (event.payload.size() == 1) {
        auto msg = std::make_shared<InstanceRunMessage>(RunType::UNSUBSCRIBE_SDK, event.payload);
        instanceRunHandler->RecvQueuePush(msg);
        msg->Wait();
        INFO(logger, "sdk " << event.payload[0] << " disconnected and has been unsubscribed related topics.");
        return eventResult;
    }
    InStream in(event.payload[0]);
    Topic topic;
    topic.Deserialize(in);
    auto msg = std::make_shared<InstanceRunMessage>(RunType::UNSUBSCRIBE,
        std::vector<std::string>{topic.GetType(), event.payload[1]});
    instanceRunHandler->RecvQueuePush(msg);
    msg->Wait();
    result = msg->result;
    eventResult.opt = Opt::UNSUBSCRIBE;
    eventResult.payload.emplace_back(Encode(result));
    return eventResult;
}
}

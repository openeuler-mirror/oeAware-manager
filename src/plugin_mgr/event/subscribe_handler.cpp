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

EventResult SubscribeHandler::Handle(const Event &event)
{
    Topic topic;
    InStream in(event.payload[0]);
    topic.Deserialize(in);
    EventResult eventResult;
    Result result;
    if (managerCallback->Subscribe(event.payload[1], topic, 0) == 0) {
        result.code = 0;
    } else {
        WARN(logger, "subscribe failed!");
        result.code = -1;
    }
    eventResult.opt = Opt::SUBSCRIBE;
    eventResult.payload.emplace_back(encode(result));
    return eventResult;
}
}
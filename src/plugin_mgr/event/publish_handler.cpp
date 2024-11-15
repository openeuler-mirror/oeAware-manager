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
#include "publish_handler.h"

namespace oeaware {
Result PublishHandler::Publish(const Event &event)
{
    auto msg = std::make_shared<InstanceRunMessage>(RunType::PUBLISH, event.payload);
    instanceRunHandler->RecvQueuePush(msg);
    msg->Wait();
    auto result = msg->result;
    return result;
}

EventResult PublishHandler::Handle(const Event &event)
{
    EventResult eventResult;
    auto result = Publish(event);
    eventResult.opt = Opt::PUBLISH;
    eventResult.payload.emplace_back(Encode(result));
    return eventResult;
}
}

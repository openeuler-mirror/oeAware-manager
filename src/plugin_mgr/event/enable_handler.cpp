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
#include "enable_handler.h"

namespace oeaware {
ErrorCode EnableHandler::InstanceEnabled(const std::string &name)
{
    if (!memoryStore->IsInstanceExist(name)) {
        return ErrorCode::ENABLE_INSTANCE_NOT_LOAD;
    }
    auto instance = memoryStore->GetInstance(name);
    if (!instance->state) {
        return ErrorCode::ENABLE_INSTANCE_UNAVAILABLE;
    }
    if (instance->enabled) {
        return ErrorCode::ENABLE_INSTANCE_ALREADY_ENABLED;
    }
    auto msg = std::make_shared<InstanceRunMessage>(RunType::ENABLED, std::vector<std::string>{instance->name});
    /* Send message to InstanceRunHandler. */
    instanceRunHandler->RecvQueuePush(msg);
    /* Wait for InstanceRunHandler to finsh this task. */
    msg->Wait();
    if (msg->result.code < 0) {
        return ErrorCode::ENABLE_INSTANCE_ENV;
    }
    return ErrorCode::OK;
}

EventResult EnableHandler::Handle(const Event &event)
{
    auto name = event.payload[0];
    auto retCode = InstanceEnabled(name);
    EventResult eventResult;
    if (retCode == ErrorCode::OK) {
        INFO(logger, name << " enabled successful.");
        eventResult.opt = Opt::RESPONSE_OK;
    } else {
        WARN(logger, name << " enabled failed. because " << ErrorText::GetErrorText(retCode) << ".");
        eventResult.opt = Opt::RESPONSE_ERROR;
        eventResult.payload.emplace_back(ErrorText::GetErrorText(retCode));
    }
    return eventResult;
}
}

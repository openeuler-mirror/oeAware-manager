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
    std::shared_ptr<Instance> instance = memoryStore->GetInstance(name);
    if (!instance->GetState()) {
        return ErrorCode::ENABLE_INSTANCE_UNAVAILABLE;
    }
    if (instance->GetEnabled()) {
        return ErrorCode::ENABLE_INSTANCE_ALREADY_ENABLED;
    }
    std::shared_ptr<InstanceRunMessage> msg = std::make_shared<InstanceRunMessage>(RunType::ENABLED, instance);
    /* Send message to InstanceRunHandler. */
    instanceRunHandler->RecvQueuePush(msg);
    /* Wait for InstanceRunHandler to finsh this task. */
    msg->Wait();
    if (msg->GetInstance()->GetEnabled()) {
        return ErrorCode::OK;
    } else {
        return ErrorCode::ENABLE_INSTANCE_ENV;
    }
}

EventResult EnableHandler::Handle(const Event &event)
{
    std::string name = event.GetPayload(0);
    auto retCode = InstanceEnabled(name);
    EventResult eventResult;
    if (retCode == ErrorCode::OK) {
        INFO(logger, name << " enabled successful.");
        eventResult.SetOpt(Opt::RESPONSE_OK);
    } else {
        WARN(logger, name << " enabled failed. because " << ErrorText::GetErrorText(retCode) << ".");
        eventResult.SetOpt(Opt::RESPONSE_ERROR);
        eventResult.AddPayload(ErrorText::GetErrorText(retCode));
    }
    return eventResult;
}
}

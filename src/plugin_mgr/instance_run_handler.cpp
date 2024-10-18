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
#include "instance_run_handler.h"
#include <thread>
#include <unistd.h>

namespace oeaware {
void InstanceRunHandler::EnableInstance(const std::string &name)
{
    auto instance = memoryStore->GetInstance(name);
    if (instance->interface->Enable() < 0) {
        WARN(logger, name << " instance enabled failed!");
        return;
    }
    instance->enabled = true;
    scheduleQueue.push(ScheduleInstance{instance, time});
}

void InstanceRunHandler::DisableInstance(const std::string &name, bool force)
{
    auto instance = memoryStore->GetInstance(name);
    instance->enabled = false;
    instance->interface->Disable();
}

bool InstanceRunHandler::HandleMessage()
{
    std::shared_ptr<InstanceRunMessage> msg;
    bool shutdown = false;
    while (this->RecvQueueTryPop(msg)) {
        std::shared_ptr<Instance> instance = msg->GetInstance();
        switch (msg->GetType()) {
            case RunType::ENABLED: {
                EnableInstance(instance->name);
                break;
            }
            case RunType::DISABLED: {
                DisableInstance(instance->name, true);
                break;
            }
            case RunType::SHUTDOWN: {
                shutdown = true;
                break;
            }
        }
        msg->NotifyOne();
        if (shutdown) {
            return false;
        }
    }
    return true;
}

void InstanceRunHandler::UpdateData()
{
    for (auto &data : managerCallback->publishData) {
        auto topic = data.topic;
        auto type = topic.GetType();
        for (auto &instanceName : managerCallback->topicInstance[type]) {
            auto instance = memoryStore->GetInstance(instanceName);
            instance->interface->UpdateData(data);
        }
    }
    managerCallback->publishData.clear();
}

void InstanceRunHandler::Schedule()
{
    while (!scheduleQueue.empty()) {
        auto schedule_instance = scheduleQueue.top();
        auto &instance = schedule_instance.instance;
        if (schedule_instance.time != time) {
            break;
        }
        scheduleQueue.pop();
        if (!instance->enabled) {
            continue;
        }
        instance->interface->Run();
        UpdateData();
        schedule_instance.time += instance->interface->GetPeriod();
        scheduleQueue.push(schedule_instance);
    }
}

void InstanceRunHandler::Start()
{
    INFO(logger, "instance schedule started!");
    const static uint64_t millisecond = 1000;
    while (true) {
        if (!HandleMessage()) {
            INFO(logger, "instance schedule shutdown!");
            break;
        }
        Schedule();
        usleep(GetCycle() * millisecond);
        AddTime(GetCycle());
    }
}

void InstanceRunHandler::Init()
{
    Logger::GetInstance().Register("InstanceSchedule");
    logger = Logger::GetInstance().Get("InstanceSchedule");
}

void InstanceRunHandler::Run()
{
    Init();
    std::thread t([this] {
        this->Start();
    });
    t.detach();
}
}

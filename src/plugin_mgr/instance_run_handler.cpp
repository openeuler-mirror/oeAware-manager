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
constexpr int INSTANCE_RUN_ONCE = 2;
constexpr int INSTANCE_RUN_ALWAYS = 1;

void InstanceRunHandler::OpenTopic()
{
    for (auto pi : managerCallback->topicInstance) {
        auto name = SplitString(pi.first, "::");
        auto instanceName = name[0];
        auto topicName = name[1];
        auto params = (name.size() > 2 ? name[2] : "");
        auto instance = memoryStore->GetInstance(instanceName);
        if (!instance->enabled) {
            instance->enabled = true;
            instance->interface->Enable();
            scheduleQueue.push(ScheduleInstance{instance, time});
        }
        instance->interface->OpenTopic(Topic{instanceName, topicName, params});
    }
    for (auto pi : managerCallback->topicSdk) {
        auto name = SplitString(pi.first, "::");
        auto instanceName = name[0];
        auto topicName = name[1];
        auto params = (name.size() > 2 ? name[2] : "");
        memoryStore->GetInstance(instanceName)->interface->OpenTopic(Topic{instanceName, topicName, params});
    }
}
Result InstanceRunHandler::EnableInstance(const std::string &name)
{
    auto instance = memoryStore->GetInstance(name);
    auto result = instance->interface->Enable();
    if (result.code < 0) {
        WARN(logger, name << " instance enabled failed!");
        return result;
    }
    // Open instance subscribed topic after enabled.
    OpenTopic();
    instance->enabled = true;
    scheduleQueue.push(ScheduleInstance{instance, time});
    INFO(logger, name << " instance enabled.");
    return result;
}

bool InstanceRunHandler::CheckInstanceDisable(const std::string &name)
{
    return managerCallback->inDegree[name].empty();
}

void InstanceRunHandler::DisableInstance(const std::string &name, bool force)
{
    if (!force && !CheckInstanceDisable(name)) {
        return;
    }
    auto instance = memoryStore->GetInstance(name);
    instance->enabled = false;
    instance->interface->Disable();
    INFO(logger, "instance " << name << " has been disabled.");
}

Result InstanceRunHandler::Subscribe(const std::vector<std::string> &payload)
{
    Topic topic{payload[0], payload[1], payload[2]};
    Result result;
    auto instance = memoryStore->GetInstance(topic.instanceName);
    if (!instance->enabled) {
        result = EnableInstance(instance->name);
        if (result.code < 0) {
            WARN(logger, "failed to start the instance of the subscription topic, instance: " << instance->name);
            return result;
        }
    }
    result = instance->interface->OpenTopic(topic);
    if (result.code < 0) {
        WARN(logger, "topic open failed, " << result.payload);
        DisableInstance(instance->name, false);
        return result;
    }
    constexpr int subscriberIndex = 3;
    result = managerCallback->Subscribe(payload[subscriberIndex], topic, 0);
    if (result.code < 0) {
        WARN(logger, "The subscribed topic " << topic.topicName << " failed.");
        return result;
    }
    if (instance->interface->GetType() & INSTANCE_RUN_ONCE) {
        topicRunOnce.emplace_back(std::make_pair(topic, payload[3]));
    }
    INFO(logger, "topic{" << LogText(topic.instanceName) << ", " << LogText(topic.topicName) << ", " <<
    LogText(topic.params) << "} has been subscribed.");
    return result;
}

void InstanceRunHandler::UpdateInDegreeIter(InDegree::iterator &pins)
{
    auto instance = memoryStore->GetInstance(pins->first);
    for (auto pt = pins->second.begin(); pt != pins->second.end();) {
        for (auto pa = pt->second.begin(); pa != pt->second.end();) {
            if (pa->second <= 0) {
                instance->interface->CloseTopic(Topic{pins->first.data(), pt->first.data(), pa->first.data()});
                pa = pt->second.erase(pa);
            } else {
                pa++;
            }
        }
        if (pt->second.empty()) {
            pt = pins->second.erase(pt);
        } else {
            pt++;
        }
    }
    if (pins->second.empty()) {
        DisableInstance(pins->first, false);
        pins = managerCallback->inDegree.erase(pins);
    } else {
        pins++;
    }
}

void InstanceRunHandler::UpdateInstance()
{
    for (auto pins = managerCallback->inDegree.begin(); pins != managerCallback->inDegree.end();) {
        UpdateInDegreeIter(pins);
    }
}

Result InstanceRunHandler::Unsubscribe(const std::vector<std::string> &payload)
{
    Result result;
    Topic topic;
    Decode(topic, payload[0]);
    result = managerCallback->Unsubscribe(payload[1], topic, 0);
    if (result.code < 0) {
        WARN(logger, result.payload);
        return result;
    }
    UpdateInstance();
    INFO(logger, "topic{" << LogText(topic.instanceName) << ", " << LogText(topic.topicName) << ", " <<
    LogText(topic.params) << "} has been unsubscribed.");
    return result;
}

Result InstanceRunHandler::UnsubscribeSdk(const std::vector<std::string> &payload)
{
    Result result;
    managerCallback->Unsubscribe(payload[0]);
    UpdateInstance();
    return result;
}

Result InstanceRunHandler::Publish(const std::vector<std::string> &payload)
{
    DataList dataList;
    InStream in(payload[0]);
    DataListDeserialize(&dataList, in);
    Topic topic{dataList.topic.instanceName, dataList.topic.topicName, dataList.topic.params};
    if (!memoryStore->IsInstanceExist(topic.instanceName)) {
        WARN(logger, "publish failed, " << "instance " << topic.instanceName << " is not exist.");
        return Result(FAILED, "instance " + topic.instanceName + " is not exist.");
    }
    auto instance = memoryStore->GetInstance(topic.instanceName);
    if (!topic.topicName.empty() && !instance->supportTopics.count(topic.topicName)) {
        WARN(logger, "publish failed, " << "instance " << topic.topicName << " is not exist.");
        return Result(FAILED, "topic " + topic.topicName + " is not exist.");
    }
    instance->interface->UpdateData(dataList);
    Result result;
    if (!instance->enabled) {
        result = EnableInstance(topic.instanceName);
        if (result.code < 0) {
            WARN(logger, result.code);
            return result;
        }
    }
    if (!topic.topicName.empty()) {
        result = instance->interface->OpenTopic(topic);
        if (result.code < 0) {
            WARN(logger, result.code);
            return result;
        }
    }
    INFO(logger, "publish applied, topic{" << topic.instanceName << ", " << topic.topicName <<"}.");
    return Result(OK);
}

bool InstanceRunHandler::HandleMessage()
{
    std::shared_ptr<InstanceRunMessage> msg;
    bool shutdown = false;
    while (this->RecvQueueTryPop(msg)) {
        DEBUG(logger, "handle message");
        switch (msg->GetType()) {
            case RunType::ENABLED: {
                EnableInstance(msg->payload[0]);
                break;
            }
            case RunType::DISABLED: {
                DisableInstance(msg->payload[0], false);
                break;
            }
            case RunType::DISABLED_FORCE: {
                DisableInstance(msg->payload[0], true);
                break;
            }
            case RunType::SUBSCRIBE: {
                msg->result = Subscribe(msg->payload);
                break;
            }
            case RunType::UNSUBSCRIBE: {
                Unsubscribe(msg->payload);
                break;
            }
            case RunType::UNSUBSCRIBE_SDK: {
                UnsubscribeSdk(msg->payload);
                break;
            }
            case RunType::PUBLISH: {
                msg->result = Publish(msg->payload);
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
        auto cTopic = data.topic;
        Topic topic{cTopic.instanceName, cTopic.topicName, cTopic.params};
        auto type = topic.GetType();
        if (!managerCallback->topicInstance.count(type)) {
            continue;
        }
        for (auto &instanceName : managerCallback->topicInstance[type]) {
            auto instance = memoryStore->GetInstance(instanceName);
            instance->interface->UpdateData(data);
        }
    }
    managerCallback->publishData.clear();
}

void InstanceRunHandler::CloseInstance(std::shared_ptr<Instance> instance)
{
    instance->enabled = false;
    instance->interface->Disable();
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
        // static plugin only run once
        if (instance->interface->GetType() & INSTANCE_RUN_ONCE) {
            CloseInstance(instance);
            continue;
        }
        schedule_instance.time += instance->interface->GetPeriod();
        scheduleQueue.push(schedule_instance);
    }
}

void InstanceRunHandler::UpdateState()
{
    for (auto &p : topicRunOnce) {
        managerCallback->Unsubscribe(p.second, p.first, 0);
    }
    UpdateInstance();
    topicRunOnce.clear();
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
        UpdateState();
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

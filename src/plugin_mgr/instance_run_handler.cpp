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

Result InstanceRunHandler::EnableInstance(const std::string &name)
{
    auto instance = memoryStore->GetInstance(name);
    auto result = instance->interface->Enable();
    if (result.code < 0) {
        WARN(logger, name << " instance enabled failed!");
        return result;
    }
    instance->enabled = true;
    instance->enableCnt++;
    if (instance->interface->GetType() & SCENARIO) {
        scheduleQueue.push(ScheduleInstance{instance, time + instance->interface->GetPeriod()});
    } else if (instance->interface->GetType() & TUNE) {
        scheduleQueue.push(ScheduleInstance{instance, time + 2 * instance->interface->GetPeriod()});
    } else {
        scheduleQueue.push(ScheduleInstance{instance, time});
    }
    INFO(logger, name << " instance enabled.");
    return result;
}

void InstanceRunHandler::DisableInstance(const std::string &name)
{
    auto instance = memoryStore->GetInstance(name);
    instance->enabled = false;
    instance->interface->Disable();
    INFO(logger, "instance " << name << " has been disabled.");
}

Result InstanceRunHandler::Subscribe(const std::vector<std::string> &payload)
{
    Topic topic = Topic::GetTopicFromType(payload[0]);
    Result result;
    constexpr int subscriberIndex = 1;
    auto instance = memoryStore->GetInstance(topic.instanceName);
    if (!instance->enabled) {
        result = EnableInstance(instance->name);
        if (result.code < 0) {
            WARN(logger, "failed to start the instance of the subscription topic, instance: " << instance->name);
            return result;
        }
    }
    if (!topicState[topic.instanceName][topic.topicName][topic.params]) {
        result = instance->interface->OpenTopic(topic);
        if (result.code < 0) {
            WARN(logger, "topic{" << LogText(topic.instanceName) << ", " << LogText(topic.topicName) << ", " <<
                LogText(topic.params) << "} open failed, " << result.payload);
            DisableInstance(instance->name);
            return result;
        }
        topicState[topic.instanceName][topic.topicName][topic.params] = true;
    }
    subscibers[payload[0]].insert(payload[subscriberIndex]);
    if (instance->interface->GetType() & INSTANCE_RUN_ONCE) {
        topicRunOnce.emplace_back(std::make_pair(topic, payload[subscriberIndex]));
    }
    INFO(logger, "topic{" << LogText(topic.instanceName) << ", " << LogText(topic.topicName) << ", " <<
    LogText(topic.params) << "} has been subscribed.");
    return result;
}


void InstanceRunHandler::UpdateInstance()
{
    for (auto &p : topicState) {
        int cntTopic = 0;
        for (auto &pt : p.second) {
            for (auto &pp : pt.second) {
                if (pp.second) {
                    Topic topic = Topic{p.first, pt.first, pp.first};
                    std::string type = Concat({p.first, pt.first, pp.first}, "::");
                    if (!subscibers.count(type)) {
                        memoryStore->GetInstance(p.first)->interface->CloseTopic(topic);
                        pp.second = false;
                    } else {
                        cntTopic++;
                    }
                }
            }
        }
        if (!cntTopic && memoryStore->GetInstance(p.first)->enabled) {
            DisableInstance(p.first);
        }
    }
}

Result InstanceRunHandler::Unsubscribe(const std::vector<std::string> &payload)
{
    Result result;
    Topic topic = Topic::GetTopicFromType(payload[0]);
    subscibers[payload[0]].erase(payload[1]);
    if (subscibers[payload[0]].empty()) {
        subscibers.erase(payload[0]);
    }
    UpdateInstance();
    INFO(logger, "topic{" << LogText(topic.instanceName) << ", " << LogText(topic.topicName) << ", " <<
    LogText(topic.params) << "} has been unsubscribed.");
    return result;
}

Result InstanceRunHandler::UnsubscribeSdk(const std::vector<std::string> &payload)
{
    Result result;
    std::string sdkFd = payload[0];
    for (auto i = subscibers.begin(); i != subscibers.end();) {
        if (i->second.count(sdkFd)) {
            i->second.erase(sdkFd);
            if (i->second.empty()) {
                i = subscibers.erase(i);
            }
            UpdateInstance();
        } else {
            ++i;
        }
    }
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

void InstanceRunHandler::PublishData(std::shared_ptr<InstanceRunMessage> &msg)
{
    for (auto subscriber : subscibers[msg->payload[0]]) {
        if (std::any_of(subscriber.begin(), subscriber.end(), ::isdigit)) {
            OutStream out;
            DataListSerialize(&msg->dataList, out);
            recvData->Push(Event(Opt::DATA, {subscriber, out.Str()}));
            continue;
        }
        Topic topic = Topic::GetTopicFromType(msg->payload[0]);
        auto instance = memoryStore->GetInstance(subscriber);
        instance->interface->UpdateData(msg->dataList);
    }
    DataListFree(&msg->dataList);
}

bool InstanceRunHandler::HandleMessage()
{
    std::shared_ptr<InstanceRunMessage> msg;
    bool shutdown = false;
    while (true) {
        if (!this->RecvQueueTryPop(msg)) {
            break;
        }
        DEBUG(logger, "handle message " << (int)msg->GetType());
        switch (msg->GetType()) {
            case RunType::ENABLED: {
                msg->result = EnableInstance(msg->payload[0]);
                break;
            }
            case RunType::DISABLED: {
                DisableInstance(msg->payload[0]);
                break;
            }
            case RunType::DISABLED_FORCE: {
                DisableInstance(msg->payload[0]);
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
            case RunType::PUBLISH_DATA: {
                PublishData(msg);
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
        // static plugin only run once
        if (instance->interface->GetType() & INSTANCE_RUN_ONCE) {
            CloseInstance(instance);
            continue;
        }
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

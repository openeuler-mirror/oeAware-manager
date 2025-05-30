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
#include "plugin.h"

namespace oeaware {
const std::string Instance::pluginEnabled = "running";
const std::string Instance::pluginDisabled = "close";
const std::string Instance::pluginStateOn = "available";
const std::string Instance::pluginStateOff = "unavailable";

int Plugin::LoadDlInstance(std::vector<std::shared_ptr<Interface>> &interfaceList)
{
    void (*getInstance)(std::vector<std::shared_ptr<Interface>>&) =
        (void(*)(std::vector<std::shared_ptr<Interface>>&))dlsym(handler, "GetInstance");
    if (getInstance == nullptr) {
        return -1;
    }
    getInstance(interfaceList);
    return 0;
}

void Plugin::SaveInstance(std::vector<std::shared_ptr<Interface>> &interfaceList)
{
    for (auto &interface : interfaceList) {
        std::shared_ptr<Instance> instance = std::make_shared<Instance>();
        std::string instanceName = interface->GetName();
        interface->SetRecvQueue(recvQueue);
        interface->SetLogger(Logger::GetInstance().Get("Plugin"));
        instance->interface = interface;
        for (auto &topic : interface->GetSupportTopics()) {
            instance->supportTopics[topic.topicName] = topic;
        }
        instance->name = instanceName;
        instance->pluginName = GetName();
        instance->enabled = false;
        instances.emplace_back(instance);
    }
}

bool Plugin::LoadInstance()
{
    std::vector<std::shared_ptr<Interface>> interfaceList;
    if (LoadDlInstance(interfaceList) < 0) {
        return false;
    }
    SaveInstance(interfaceList);
    return true;
}

int Plugin::Load(const std::string &dl_path)
{
    this->handler = dlopen(dl_path.c_str(), RTLD_LAZY);
    if (handler == nullptr) {
        return -1;
    }
    if (!LoadInstance()) {
        return -1;
    }
    return 0;
}

std::string Instance::GetInfo() const
{
    std::string stateText = this->state ? pluginStateOn : pluginStateOff;
    std::string runText = this->enabled ? pluginEnabled : pluginDisabled;
    return name + "(" + stateText + ", " + runText + ", count: " + std::to_string(enableCnt) + ")";
}

std::string Instance::GetRun() const
{
    std::string runText = this->enabled ? pluginEnabled : pluginDisabled;
    return name + " (" + runText + ")";
}

Result Instance::OpenTopic(const Topic &topic)
{
    Result ret = interface->OpenTopic(topic);
    if (ret.code == OK) {
        openTopics.insert(topic.GetType());
    }
    return ret;
}

void Instance::CloseTopic(const Topic &topic)
{
    interface->CloseTopic(topic);
    openTopics.erase(topic.GetType());
}

void Instance::Disable()
{
    std::vector<std::string> topicsTypes = std::vector<std::string>(openTopics.begin(), openTopics.end());
    for (const auto &type : topicsTypes) {
        interface->CloseTopic(Topic::GetTopicFromType(type));
    }
    interface->Disable();
}

std::string Instance::GetName() const
{
    return name;
}
}

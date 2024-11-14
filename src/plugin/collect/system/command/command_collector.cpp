/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/

#include "command_collector.h"

CommandCollector::CommandCollector(): oeaware::Interface()
{
    this->name = "command_collector";
    this->version = "1.0.0";
    this->description = "collect information of command";
    this->priority = 0;
    this->type = 0;
    constexpr int RUN_PERIOD = 10;
    this->period = RUN_PERIOD;
    for (const auto &iter : topicStr) {
        oeaware::Topic topic;
        topic.instanceName = this->name;
        topic.topicName = iter;
        supportTopics.push_back(topic);
    }
}

void CommandCollector::CollectThread(const oeaware::Topic &topic, CommandBase* collector)
{
    std::string cmd = collector->GetCommand(topic);
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: Failed to execute command: " << cmd << std::endl;
        return;
    }
    char buffer[256];
    while (collector->isRunning && fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        collector->ParseLine(std::string(buffer));
    }
    pclose(pipe);
    
    collector->Close();
}

void CommandCollector::PublishThread(const oeaware::Topic &topic, CommandBase* collector)
{
    while (collector->isRunning) {
        if (collector->hasNewData.exchange(false)) {
            DataList dataList;
            dataList.topic.instanceName = new char[name.size() + 1];
            if (strcpy_s(dataList.topic.instanceName, name.size() + 1, name.c_str()) != EOK) {
                continue;
            }
            dataList.topic.topicName = new char[topic.topicName.size() + 1];
            if (strcpy_s(dataList.topic.topicName, topic.topicName.size() + 1, topic.topicName.c_str()) != EOK) {
                continue;
            }
            dataList.topic.params = new char[topic.params.length() + 1];
            if (strcpy_s(dataList.topic.params, topic.params.length() + 1, topic.params.c_str()) != EOK) {
                continue;
            }
            if (!collector->FillDataStruct(&dataList)) {
                continue;
            }
            SarData *pdata = (SarData *)dataList.data[0];
            Publish(dataList);
            collector->FreeData(&dataList);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

oeaware::Result CommandCollector::OpenTopic(const oeaware::Topic &topic)
{
    if (!CommandBase::ValidateArgs(topic)) {
        return oeaware::Result(FAILED, "params invalid.");
    }
    auto topicType = topic.GetType();
    std::lock_guard<std::mutex> lock(topicMutex[topicType]);
    auto it = collectors.find(topicType);
    if (it == collectors.end()) {
        collectors[topicType] = std::make_unique<CommandBase>();
        collectors[topicType]->topic = topic;
    }
    auto &cmd = collectors[topicType];
    if (cmd->isRunning) {
        return oeaware::Result(OK);
    }
    cmd->isRunning = true;
    collectThreads[topicType] = std::thread(&CommandCollector::CollectThread, this, topic, cmd.get());
    publishThreads[topicType] = std::thread(&CommandCollector::PublishThread, this, topic, cmd.get());
    return oeaware::Result(OK);
}

void CommandCollector::CloseTopic(const oeaware::Topic &topic)
{
    std::thread t([this, topic] {
        auto topicType = topic.GetType();
        std::lock_guard<std::mutex> lock(topicMutex[topicType]);
        auto it = collectors.find(topicType);
        if (it != collectors.end() && it->second->isRunning) {
            it->second->isRunning = false;
            if (collectThreads[topicType].joinable()) {
                collectThreads[topicType].join();
                collectThreads.erase(topicType);
            }
            if (publishThreads[topicType].joinable()) {
                publishThreads[topicType].join();
                publishThreads.erase(topicType);
            }
        }
    });
    t.detach();
}

oeaware::Result CommandCollector::Enable(const std::string &parma)
{
    return oeaware::Result(OK);
}

void CommandCollector::Disable()
{
    return;
}

void CommandCollector::UpdateData(const DataList &dataList)
{
    return;
}

void CommandCollector::Run()
{
    return;
}

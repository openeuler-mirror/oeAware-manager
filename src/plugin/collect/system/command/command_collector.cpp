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
    this->period = 0;

    collectors["mpstat"] = std::make_unique<MpstatCollector>();
    collectors["iostat"] = std::make_unique<IostatCollector>();
    collectors["vmstat"] = std::make_unique<VmstatCollector>();

    for (const auto &iter : topicStr) {
        oeaware::Topic topic;
        topic.instanceName = this->name;
        topic.topicName = iter;
        supportTopics.push_back(topic);
    }
}

void CommandCollector::CollectThread(const oeaware::Topic &topic, CommandBase* collector)
{
    while (collector->isRunning) {
        std::string cmd = collector->GetCommand(topic.params);
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
    }
}

void CommandCollector::PublishThread(const oeaware::Topic &topic, CommandBase* collector)
{
    while (collector->isRunning) {
        if (collector->hasNewData.exchange(false)) {
            void* data = collector->CreateDataStruct();
            if (!data) {
                continue;
            }

            std::lock_guard<std::mutex> lock(collector->dataMutex);
            if (!collector->FillDataStruct(data)) {
                continue;
            }

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
            dataList.data = new void* [1];
            dataList.len = 1;
            dataList.data[0] = data;
            Publish(dataList);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

oeaware::Result CommandCollector::OpenTopic(const oeaware::Topic &topic)
{
    auto it = collectors.find(topic.topicName);
    if (it != collectors.end() && it->second->ValidateArgs(topic.params)) {
        it->second->isRunning = true;
        collectThreads[topic.topicName] = std::thread(&CommandCollector::CollectThread,
                                                      this, topic, it->second.get());
        publishThreads[topic.topicName] = std::thread(&CommandCollector::PublishThread,
                                                      this, topic, it->second.get());
        return oeaware::Result(0);
    }
    return oeaware::Result(-1, "Unsupported topic");
}

void CommandCollector::CloseTopic(const oeaware::Topic &topic)
{
    auto it = collectors.find(topic.topicName);
    if (it != collectors.end()) {
        it->second->isRunning = false;
        std::lock_guard<std::mutex> lock(it->second->dataMutex);
        it->second->data.clear();
        it->second->hasNewData = false;

        if (collectThreads[topic.topicName].joinable()) {
            collectThreads[topic.topicName].join();
            collectThreads.erase(topic.topicName);
        }
        if (publishThreads[topic.topicName].joinable()) {
            publishThreads[topic.topicName].join();
            publishThreads.erase(topic.topicName);
        }
    }
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

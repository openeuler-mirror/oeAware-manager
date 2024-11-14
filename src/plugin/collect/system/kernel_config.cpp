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
#include "kernel_config.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>
#include <securec.h>

KernelConfig::KernelConfig(): oeaware::Interface()
{
    this->name = "kernel_config";
    this->version = "1.0.0";
    this->description = "collect or set kernel config";
    this->priority = 0;
    this->type = 2;
    this->period = 0;
    for (auto &iter : topicStr) {
        oeaware::Topic topic;
        topic.instanceName = this->name;
        topic.topicName = iter;
        supportTopics.push_back(topic);
    }
}

oeaware::Result KernelConfig::OpenTopic(const oeaware::Topic &topic)
{
    if (find(topicStr.begin(), topicStr.end(), topic.topicName) == topicStr.end()) {
        return oeaware::Result(FAILED, "topic{" + topic.topicName + "} invalid.");
    }
    std::stringstream ss(topic.params);
    std::string word;
    while (ss >> word) {
        if (kernelParams.count(word)) {
            if (topic.topicName == "get_kernel_config") {
                getTopics[topic.GetType()].insert(word);
            } else {
                setTopics[topic.GetType()].insert(word);
            }
        }
    }
    return oeaware::Result(OK);
}

void KernelConfig::CloseTopic(const oeaware::Topic &topic)
{
    getTopics.erase(topic.GetType());
    setTopics.clear();
}

oeaware::Result KernelConfig::Enable(const std::string &param)
{
    (void)param;
    FILE *pipe = popen("sysctl -a", "r");
    if (!pipe) {
        return oeaware::Result(FAILED, "Error: popen() filed");
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        std::string str = buffer;
        auto values = oeaware::SplitString(str, " = ");
        constexpr int paramsCnt = 2;
        if (values.size() != paramsCnt) {
            continue;
        }
        kernelParams[values[0]] = values[1];
    }
    pclose(pipe);
    return oeaware::Result(OK);
}

void KernelConfig::Disable()
{
    kernelParams.clear();
    setTopics.clear();
    getTopics.clear();
    return;
}

void KernelConfig::UpdateData(const DataList &dataList)
{
    for (int i = 0; i < dataList.len; ++i) {
        KernelData *kernelData = (KernelData*)dataList.data[i];
        auto tmp = kernelData->kernelData;
        for (int j = 0; j < kernelData->len; ++j) {
            std::string key(tmp->key);
            std::string value(tmp->value);
            setTopics[key].insert(value);
            tmp = tmp->next;
        }
    }
    return;
}

void KernelConfig::PublishKernelConfig()
{
    if (getTopics.empty()) {
        return;
    }
    for (auto &p : getTopics) {
        oeaware::Topic topic = oeaware::Topic::GetTopicFromType(p.first);
        DataList dataList;
        dataList.topic.instanceName = new char[topic.instanceName.size() + 1];
        strcpy_s(dataList.topic.instanceName, topic.instanceName.size() + 1, topic.instanceName.data());
        dataList.topic.topicName = new char[topic.topicName.size() + 1];
        strcpy_s(dataList.topic.topicName, topic.topicName.size() + 1, topic.topicName.data());
        dataList.topic.params = new char[topic.params.size() + 1];
        strcpy_s(dataList.topic.params, topic.params.size() + 1, topic.params.data());
        KernelData *data = new KernelData();
        KernelDataNode *tmp = nullptr;
        for (auto &name : p.second) {
            KernelDataNode *newNode = createNode(name.data(), kernelParams[name].data());
            if (data->kernelData == NULL) {
                data->kernelData = newNode;
                tmp = newNode;
            } else {
                tmp->next = newNode;
                tmp = tmp->next;
            }
            data->len++;
        }

        dataList.len = 1;
        dataList.data = new void* [1];
        dataList.data[0] = data;
        Publish(dataList);
    }
}

void KernelConfig::SetKernelConfig()
{
    for (auto &p : setTopics) {
        for (auto &value : p.second) {
            std::string cmd = "sysctl -w " + p.first + "=" + value;
            FILE *pipe = popen(cmd.data(), "r");
            if (!pipe) {
                WARN(logger, "set kernel config{" << p.first << "=" << value << "} failed.");
                continue;
            }
            pclose(pipe);
        }
    }
}

void KernelConfig::Run()
{
    PublishKernelConfig();
    SetKernelConfig();
}

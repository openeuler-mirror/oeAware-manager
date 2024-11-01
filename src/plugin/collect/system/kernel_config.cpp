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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>
#include <securec.h>
#include "kernel_config.h"

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

int KernelConfig::OpenTopic(const oeaware::Topic &topic)
{
    params = topic.params;
    FILE *pipe = popen("sysctl -a", "r");
    if (!pipe) {
        std::cout << "Error: popen() filed" << std::endl;
        return -1;
    }

    char buffer[1024];
    size_t pos;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        std::string str = buffer;
        kernelInfo.push_back(str);
    }

    pclose(pipe);
    return 0;
}

void KernelConfig::CloseTopic(const oeaware::Topic &topic)
{
    kernelInfo.clear();
}

int KernelConfig::Enable(const std::string &parma)
{
    return 0;
}

void KernelConfig::Disable()
{
    return;
}

void KernelConfig::UpdateData(const oeaware::DataList &dataList)
{
    return;
}

void KernelConfig::searchRegex(const std::string &regexStr, std::shared_ptr <KernelData> &data)
{
    std::regex regexObj(regexStr);
    std::smatch match;

    for (auto &iter : kernelInfo) {
        if (regex_search(iter, match, regexObj)) {
            size_t pos = iter.find("=");
            KernelDataNode *newNode = createNode(iter.substr(0, pos - 1).c_str(), iter.substr(pos + 2).c_str());
            if (data->kernelData == NULL) {
                data->kernelData = newNode;
            } else {
                auto tmp = data->kernelData;
                while (tmp->next != NULL) {
                    tmp = tmp->next;
                }
                tmp->next = newNode;
            }
            data->len++;
        }
    }
}

void KernelConfig::Run()
{
    std::shared_ptr <KernelData> data = std::make_shared<KernelData>();
    std::istringstream iss(params);
    std::string token;
    while (getline(iss, token, ' ')) {
        searchRegex(token, data);
    }

    struct oeaware::DataList dataList;
    dataList.topic.instanceName = this->name;
    dataList.topic.topicName = "get_kernel_config";
    dataList.data.push_back(data);
    Publish(dataList);
}
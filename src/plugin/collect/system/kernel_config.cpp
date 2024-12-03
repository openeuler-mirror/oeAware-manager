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
#include <dirent.h>
#include <sys/stat.h>
#include "oeaware/utils.h"
#include "command_base.h"
#include "oeaware/data/kernel_data.h"
#include "data_register.h"

KernelConfig::KernelConfig(): oeaware::Interface()
{
    this->name = OE_KERNEL_CONFIG_COLLECTOR;
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

bool KernelConfig::InitCmd(std::stringstream &ss, const std::string &topicType)
{
    std::string cmd;
    std::string word;
    while (ss >> word) {
        if (word == cmdSeparator) {
            if (!CommandBase::ValidateCmd(cmd)) {
                return false;
            }
            getCmds[topicType].emplace_back(cmd);
            cmd = "";
            continue;
        }
        if (!cmd.empty()) {
            cmd += " ";
        }
        cmd += word;
    }
    if (!CommandBase::ValidateCmd(cmd)) {
        return false;
    }
    getCmds[topicType].emplace_back(cmd);
    return true;
}

oeaware::Result KernelConfig::OpenTopic(const oeaware::Topic &topic)
{
    if (find(topicStr.begin(), topicStr.end(), topic.topicName) == topicStr.end()) {
        return oeaware::Result(FAILED, "topic{" + topic.topicName + "} invalid.");
    }
    std::stringstream ss(topic.params);
    std::string word;
    std::string topicType = topic.GetType();
    if (topic.topicName == "get_cmd" && !InitCmd(ss, topicType)) {
        return oeaware::Result(FAILED, "params invalid.");
    } else if (topic.topicName == "get_kernel_config") {
        while (ss >> word) {
            getTopics[topicType].insert(word);
        }
    }
    return oeaware::Result(OK);
}

void KernelConfig::CloseTopic(const oeaware::Topic &topic)
{
    getTopics.erase(topic.GetType());
    getCmds.erase(topic.GetType());
    setSystemParams.clear();
    cmdRun.clear();
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
        sysctlParams[values[0]] = values[1];
    }
    pclose(pipe);
    return oeaware::Result(OK);
}

void KernelConfig::Disable()
{
    sysctlParams.clear();
    setSystemParams.clear();
    getTopics.clear();
    cmdRun.clear();
    return;
}

void KernelConfig::UpdateData(const DataList &dataList)
{
    for (uint64_t i = 0; i < dataList.len; ++i) {
        KernelData *kernelData = (KernelData*)dataList.data[i];
        auto tmp = kernelData->kernelData;
        for (int j = 0; j < kernelData->len; ++j) {
            std::string key(tmp->key);
            std::string value(tmp->value);
            if (value.empty()) {
                cmdRun.emplace_back(key);
            } else {
                setSystemParams.emplace_back(std::make_pair(key, value));
            }
            tmp = tmp->next;
        }
    }
    return;
}

std::vector<std::string> KernelConfig::getCmdGroup{"cat", "grep", "awk", "pgrep", "ls", "ethtool"};

static void SetKernelData(KernelData *data, const std::string &ret)
{
    data->kernelData = new KernelDataNode();
    if (data->kernelData == nullptr) {
        return;
    }
    data->len = 1;
    data->kernelData->key = new char[1];
    if (data->kernelData->key == nullptr) {
        delete data->kernelData;
        data->kernelData = nullptr;
        return;
    }
    data->kernelData->key[0] = 0;
    data->kernelData->value = new char[ret.size() + 1];
    if (data->kernelData->value == nullptr) {
        delete data->kernelData;
        delete[] data->kernelData->key;
        data->kernelData->key = nullptr;
        data->kernelData = nullptr;
        return;
    }
    strcpy_s(data->kernelData->value, ret.size() + 1, ret.data());
    data->kernelData->next = nullptr;
}

void KernelConfig::PublishCmd()
{
    for (auto &p : getCmds) {
        oeaware::Topic topic = oeaware::Topic::GetTopicFromType(p.first);
        DataList dataList;
        if (!oeaware::SetDataListTopic(&dataList, topic.instanceName, topic.topicName, topic.params)) {
            continue;
        }
        KernelData *data = new KernelData();
        if (data == nullptr) {
            WARN(logger, "KernelData failed to allocate memory.");
            continue;
        }
        std::string cmd = "";
        for (auto &cmdPart : p.second) {
            if (!cmd.empty()) {
                cmd += " | ";
            }
            cmd += cmdPart;
        }
        PopenProcess pipe;
        pipe.Popen(cmd);
        char buffer[1024];
        std::string ret = "";
        while (fgets(buffer, sizeof(buffer), pipe.stream) != nullptr) {
            ret += buffer;
        }
        if (pipe.Pclose() < 0) {
            WARN(logger, "pipe close error.");
        }
        SetKernelData(data, ret);
        dataList.len = 1;
        dataList.data = new void* [1];
        if (dataList.data == nullptr) {
            oeaware::Register::GetInstance().GetDataFreeFunc("kernel_config")(data);
            continue;
        }
        dataList.data[0] = data;
        Publish(dataList);
    }
}

void KernelConfig::PublishKernelParams()
{
    for (auto &p : getTopics) {
        oeaware::Topic topic = oeaware::Topic::GetTopicFromType(p.first);
        DataList dataList;
        if (!oeaware::SetDataListTopic(&dataList, topic.instanceName, topic.topicName, topic.params)) {
            continue;
        }
        KernelData *data = new KernelData();
        if (data == nullptr) {
            WARN(logger, "KernelData failed to allocate memory.");
            continue;
        }
        KernelDataNode *tmp = nullptr;
        for (auto &name : p.second) {
            std::string value = "";
            if (sysctlParams.count(name)) {
                value = sysctlParams[name];
            } else {
                WARN(logger, "invalid params: " << name << ".");
                continue;
            }
            KernelDataNode *newNode = CreateNode(name.data(), value.data());
            if (newNode == nullptr) {
                WARN(logger, "KernelDataNode failed to allocate memory.");
                continue;
            }
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

void KernelConfig::PublishKernelConfig()
{
    if (getTopics.empty() && getCmds.empty()) {
        return;
    }
    PublishCmd();
    PublishKernelParams();
}

void KernelConfig::WriteSysParam(const std::string &path, const std::string &value)
{
    std::ofstream sysFile(path);
    if (!sysFile.is_open()) {
        WARN(logger, "failed to write to : " << path << ".");
        return;
    }
    sysFile << value;
    if (sysFile.fail()) {
        WARN(logger, "failed to write to : " << path << ".");
        return;
    }
    sysFile.close();
     if (sysFile.fail()) {
        WARN(logger, "failed to write to : " << path << ".");
        return;
    }
    INFO(logger, "successfully wrote value{" << value <<"} to "  << path << ".");
}

std::vector<std::string> KernelConfig::cmdGroup{"sysctl", "ifconfig", "/sbin/blockdev"};

void KernelConfig::SetKernelConfig()
{
    for (auto &p : setSystemParams) {
        WriteSysParam(p.first, p.second);
    }
    for (auto &cmd : cmdRun) {
        auto cmdParts = oeaware::SplitString(cmd, " ");
        if (cmdParts.empty() || std::find(cmdGroup.begin(), cmdGroup.end(), cmdParts[0]) == cmdGroup.end() ||
            !CommandBase::ValidateCmd(cmd)) {
            WARN(logger, "cmd{" << cmd << "} invalid.");
            continue;
        }
        FILE *pipe = popen(cmd.data(), "r");
        if (!pipe) {
            WARN(logger, "{" << cmd << "} run failed.");
            continue;
        }
        INFO(logger, "{" << cmd << "} run successfully.");
        pclose(pipe);
    }
}

void KernelConfig::Run()
{
    PublishKernelConfig();
    SetKernelConfig();
}

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
#include "command_base.h"

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
        if (topic.topicName == "get_kernel_config") {
            getTopics[topic.GetType()].insert(word);
        }
    }
    return oeaware::Result(OK);
}

void KernelConfig::CloseTopic(const oeaware::Topic &topic)
{
    getTopics.erase(topic.GetType());
    setSystemParams.clear();
    cmdRun.clear();
}

void KernelConfig::InitFileParam()
{
    for (auto &v : kernelParamPath) {
        std::string path = v[1];
        std::ifstream file(path);
        if (!file.is_open()) {
            continue;
        }
        std::string key = v[0];
        std::string line;
        std::string value = "";
        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }
            value += line;
            value += '\n';
        }
        kernelParams[key] = value;
        file.close();
    }
}

void KernelConfig::AddCommandParam(const std::string &cmd)
{
    FILE *pipe = popen(cmd.data(), "r");
    if (!pipe) {
        return;
    }
    char buffer[1024];
    std::string value;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        value += buffer;
    }
    pclose(pipe);
    auto v = oeaware::SplitString(cmd, " ");
    std::vector<std::string> skipSpace;
    for (auto &word : v) {
        if (word.empty()) continue;
        skipSpace.emplace_back(word);
    }
    if (skipSpace.size() > 1 && v[0] == "ethtool") {
        kernelParams[v[0] + "@" + v[1]] = value;
        return;
    }
    kernelParams[cmd] = value;
}

static bool IsSymlink(const std::string &path)
{
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) {
        perror("lstat failed");
        return false;
    }
    return S_ISLNK(st.st_mode);
}

void KernelConfig::GetAllEth()
{
    const std::string path = "/sys/class/net";
    std::vector<std::string> interfaces;
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        WARN(logger, "failed to open directory: " << path << ".");
        return;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name(entry->d_name);
        if (name != "." && name != ".." && IsSymlink(path + "/" + name)) {
            allEths.push_back(name);
        }
    }
    closedir(dir);
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
    InitFileParam();
    GetAllEth();
    AddCommandParam("lscpu");
    AddCommandParam("ifconfig");
    for (auto &eth : allEths) {
        AddCommandParam("ethtool " + eth);
    }

    return oeaware::Result(OK);
}

void KernelConfig::Disable()
{
    sysctlParams.clear();
    kernelParams.clear();
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
            std::string value = "";
            if (sysctlParams.count(name)) {
                value = sysctlParams[name];
            } else if (kernelParams.count(name)) {
                value = kernelParams[name];
            } else {
                continue;
            }
            KernelDataNode *newNode = createNode(name.data(), value.data());
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

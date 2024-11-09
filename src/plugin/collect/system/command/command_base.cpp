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
#include "command_base.h"

CommandBase::CommandBase()
{
    attrsFirst["sar"] = {"pgpgin/s", "kbmemfree", "runq-sz", "DEV", "IFACE"};
    attrsFirst["iostat"] = {"Device", "avg-cpu:"};
    attrsFirst["mpstat"] = {"CPU"};
    attrsFirst["pidstat"] = {"UID"};
    attrsFirst["vmstat"] = {"swpd"};
}

bool CommandBase::ValidateArgs(const oeaware::Topic& topic)
{
    auto cmd = GetCommand(topic);
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return false;
    }
    char buffer[128];
    bool isValid = false;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        if (strstr(buffer, "Linux") != nullptr || strstr(buffer, "procs") != nullptr) {
            isValid = true;
        }
    }
    pclose(pipe);
    return isValid;
}

void CommandBase::ParseLine(const std::string& line)
{
    // Skip lines that are not needed.
    if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos ||
        line.find("Linux") != std::string::npos) {
        return;
    }
    for (auto &skip : skipLine) {
        if (line.find(skip) != std::string::npos) {
            return;
        }
    }
    std::istringstream iss(line);
    bool isAttr = false;
    // Check whether it is an attribute line.
    for (auto &attr : attrsFirst[topic.topicName]) {
        if (line.find(attr) != std::string::npos) {
            nowType = topic.GetType() + attr;
            isAttr = true;
            break;
        }
    }
    if (isAttr) {
        if (dataTypes.count(nowType)) {
            return;
        }
        std::string name;
        while (iss >> name) {
            if (name == "avg-cpu:") {
                continue;
            }
            dataTypes[nowType].emplace_back(name);
        }
        return;
    }
    std::string value;
    std::vector<std::string> values;
    while (iss >> value) {
        values.emplace_back(value);
    }
    std::lock_guard<std::mutex> lock(dataMutex);
    datas[nowType].emplace_back(values);
    hasNewData = true;
}

std::string CommandBase::GetCommand(const oeaware::Topic& topic)
{
    return topic.topicName + " " + topic.params;
}

bool CommandBase::FillDataStruct(void* dataStruct)
{
    DataList *dataList = (DataList *)dataStruct;
    int len = 0;
    std::vector<CommandData*> commandDatas;
    std::lock_guard<std::mutex> lock(dataMutex);
    int ret;
    for (auto& pair : datas) {
        if (pair.second.empty()) {
            continue;
        }
        auto* commandData = new CommandData();
        commandData->attrLen = dataTypes[pair.first].size();
        commandData->itemLen = pair.second.size();
        commandData->items = new CommandIter[commandData->itemLen];
        for (int i = 0; i < commandData->attrLen; ++i) {
            auto attr = dataTypes[pair.first][i];
            
            commandData->itemAttr[i] = new char[attr.size() + 1];
            ret = strcpy_s(commandData->itemAttr[i], attr.size() + 1, attr.data());
            if (ret != EOK) {
                return false;
            }
        }
        for (int i = 0; i < commandData->itemLen; ++i) {
            auto item = pair.second[i];
            for (int j = 0; j < item.size(); ++j) {
                commandData->items[i].value[j] = new char[item[j].size() + 1];
                ret = strcpy_s(commandData->items[i].value[j], item[j].size() + 1, item[j].data());
                if (ret != EOK) {
                    return false;
                }
            }
        }
        len++;
        commandDatas.emplace_back(commandData);
        pair.second.clear();
    }
    dataList->len = len;
    dataList->data = new void* [len];
    for (int i = 0; i < len; ++i) {
        dataList->data[i] = commandDatas[i];
    }
    return true;
}

bool CommandBase::FreeData(void *dataList)
{
    return true;
}

void CommandBase::Close()
{
    datas.clear();
}

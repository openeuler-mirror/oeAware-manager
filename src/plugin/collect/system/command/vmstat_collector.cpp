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

#include "vmstat_collector.h"

bool VmstatCollector::ValidateArgs(const std::string& args)
{
    std::istringstream iss(args);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() != 1) {
        return false;
    }

    try {
        if (tokens[0].find_first_not_of("0123456789") != std::string::npos) {
            std::cerr << "Period must contain only digits" << std::endl;
            return false;
        }
        int period = std::stoi(tokens[0]);
        if (period <= 0) {
            std::cerr << "Period must be a positive number" << std::endl;
            return false;
        }
    } catch (const std::exception&) {
        std::cerr << "Invalid period format" << std::endl;
        return false;
    }
    return true;
}

void VmstatCollector::ParseLine(const std::string& line)
{
    if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos ||
        line.find("procs") != std::string::npos) {
        return;
    }

    std::istringstream iss(line);
    if (line.find("free") != std::string::npos) {
        std::string name;
        while (iss >> name) {
            names.push_back(name);
        }
        return;
    }

    double value;
    std::vector<double> values;
    while (iss >> value) {
        values.push_back(value);
    }

    std::lock_guard<std::mutex> lock(dataMutex);
    for (size_t i = 0; i < names.size(); i++) {
        data[names[i]] = values[i];
    }
    hasNewData = true;
}

std::string VmstatCollector::GetCommand(const std::string& params)
{
    return "vmstat " + params;
}

void* VmstatCollector::CreateDataStruct()
{
    return new VmstatData;
}

bool VmstatCollector::FillDataStruct(void* dataStruct)
{
    auto* vmstatData = static_cast<VmstatData*>(dataStruct);
    int len = data.size();
    vmstatData->vmstatArray = new CommandArray[len];

    vmstatData->len = 0;
    for (const auto& pair : data) {
        vmstatData->vmstatArray[vmstatData->len].key = new char[pair.first.length() + 1];
        auto ret = strcpy_s(vmstatData->vmstatArray[vmstatData->len].key, pair.first.length() + 1, pair.first.c_str());
        if (ret != EOK) {
            break;
        }
        vmstatData->vmstatArray[vmstatData->len].key[pair.first.length()] = '\0';
        vmstatData->vmstatArray[vmstatData->len].value = pair.second;
        vmstatData->len++;
    }
    return true;
}

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

#include "iostat_collector.h"

bool IostatCollector::ValidateArgs(const std::string& args)
{
    std::istringstream iss(args);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() != 2 || tokens[0] != "-d") {
        return false;
    }

    try {
        int period = std::stoi(tokens[1]);
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

void IostatCollector::ParseLine(const std::string& line)
{
    if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos ||
        line.find("Linux") != std::string::npos) {
        return;
    }

    std::istringstream iss(line);
    if (line.find("Device") != std::string::npos) {
        if (names.empty()) {
            std::string name;
            while (iss >> name) {
                names.push_back(name);
            }
        }
        return;
    }

    std::string device;
    double value;
    std::vector<double> values;

    iss >> device;
    while (iss >> value) {
        values.push_back(value);
    }

    std::lock_guard<std::mutex> lock(dataMutex);
    for (size_t i = 0; i < names.size(); i++) {
        data[names[i] + "@" + device] = values[i];
    }
    hasNewData = true;
}

std::string IostatCollector::GetCommand(const std::string& params)
{
    return "iostat " + params;
}

void* IostatCollector::CreateDataStruct()
{
    return new IostatData;
}

bool IostatCollector::FillDataStruct(void* dataStruct)
{
    auto* iostatData = static_cast<IostatData*>(dataStruct);
    int len = data.size();
    iostatData->iostatArray = new CommandArray[len];

    iostatData->len = 0;
    for (const auto& pair : data) {
        iostatData->iostatArray[iostatData->len].key = new char[pair.first.length() + 1];
        auto ret = strcpy_s(iostatData->iostatArray[iostatData->len].key, pair.first.length() + 1, pair.first.c_str());
        if (ret != EOK) {
            break;
        }
        iostatData->iostatArray[iostatData->len].key[pair.first.length()] = '\0';
        iostatData->iostatArray[iostatData->len].value = pair.second;
        iostatData->len++;
    }
    return true;
}

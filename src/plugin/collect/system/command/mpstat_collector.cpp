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

#include "mpstat_collector.h"

bool MpstatCollector::ValidateArgs(const std::string& args)
{
    std::istringstream iss(args);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() != 4) {
        std::cerr << "Invalid argument count. Format should be: -u -P <cpus> <period>" << std::endl;
        return false;
    }

    if (tokens[0] != "-u" || tokens[1] != "-P") {
        std::cerr << "First two arguments must be '-u -P'" << std::endl;
        return false;
    }

    if (tokens[2] != "ALL") {
        std::istringstream cpu_iss(tokens[2]);  // 分割逗号分隔的CPU列表
        std::string cpu_part;
        while (std::getline(cpu_iss, cpu_part, ',')) {
            if (cpu_part.find('-') != std::string::npos) {  // 处理范围格式 (例如 "0-64")
                size_t dashPos = cpu_part.find('-');
                std::string start = cpu_part.substr(0, dashPos);
                std::string end = cpu_part.substr(dashPos + 1);

                if (start.empty() || end.empty() ||
                    start.find_first_not_of("0123456789") != std::string::npos ||
                    end.find_first_not_of("0123456789") != std::string::npos ||
                    std::stoi(start) >= std::stoi(end)) {
                    std::cerr << "Invalid CPU range format in: " << cpu_part << std::endl;
                    return false;
                }
            } else if (cpu_part.find_first_not_of("0123456789") != std::string::npos) { // 检查是否为纯数字
                std::cerr << "Invalid CPU number format in: " << cpu_part << std::endl;
                return false;
            }
        }
    }

    try {
        if (tokens[3].find_first_not_of("0123456789") != std::string::npos) {
            std::cerr << "Period must contain only digits" << std::endl;
            return false;
        }
        int period = std::stoi(tokens[3]);
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

void MpstatCollector::ParseLine(const std::string& line)
{
    if (line.find("AM") == std::string::npos && line.find("PM") == std::string::npos) {
        return;
    }

    std::istringstream iss(line);
    std::string time, meridiem, cpu;
    std::vector<double> values;

    iss >> time >> meridiem >> cpu;

    if (cpu == "CPU") {
        if (names.empty()) {
            std::string name;
            while (iss >> name) {
                names.push_back(name);
            }
        }
        return;
    }

    double value;
    while (iss >> value) {
        values.push_back(value);
    }

    std::string cpuName;
    if (cpu == "all") {
        cpuName = "all";
    } else {
        try {
            if (cpu.find_first_not_of("0123456789") == std::string::npos) {
                cpuName = std::to_string(std::stoi(cpu));
            } else {
                return;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error parsing CPU number: " << e.what() << ", cpu = " << cpu << std::endl;
            return;
        }
    }

    std::lock_guard<std::mutex> lock(dataMutex);
    for (size_t i = 0; i < names.size(); i++) {
        data[names[i] + "@cpu-" + cpuName] = values[i];
    }
    hasNewData = true;
}

std::string MpstatCollector::GetCommand(const std::string& params)
{
    return "mpstat " + params;
}

void* MpstatCollector::CreateDataStruct()
{
    return new MpstatData;
}

bool MpstatCollector::FillDataStruct(void* dataStruct)
{
    auto* mpstatData = static_cast<MpstatData*>(dataStruct);
    int len = data.size();
    mpstatData->mpstatArray = new CommandArray[len];

    mpstatData->len = 0;
    for (const auto& pair : data) {
        mpstatData->mpstatArray[mpstatData->len].key = new char[pair.first.length() + 1];
        auto ret = strcpy_s(mpstatData->mpstatArray[mpstatData->len].key, pair.first.length() + 1, pair.first.c_str());
        if (ret != EOK) {
            break;
        }
        mpstatData->mpstatArray[mpstatData->len].key[pair.first.length()] = '\0';
        mpstatData->mpstatArray[mpstatData->len].value = pair.second;
        mpstatData->len++;
    }
    return true;
}

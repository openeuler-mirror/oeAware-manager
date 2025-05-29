/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include "irq_frontend.h"
#include <regex>
#include <fstream>
#include <iostream>
#include "oeaware/utils.h"

void IrqFrontEnd::GetEthQueData(std::unordered_map<std::string, EthQueInfo> &ethQueData, const std::vector<std::string> &queueRegex)
{
    std::unordered_map<int, std::string> irqWithDesc;
    GetIrqWithDesc(irqWithDesc);
    for (auto &item : ethQueData) {
        EthQueInfo &info = item.second;
        info.AddRegex(queueRegex);
        info.UpdateQue2Irq(irqWithDesc);
        info.ProcSpecialEth();
    }
}

void IrqFrontEnd::GetIrqWithDesc(std::unordered_map<int, std::string> &irqWithDesc)
{
    std::ifstream file("/proc/interrupts");
    if (!file) {
        std::cerr << "Failed to open file." << std::endl;
        return;
    }

    std::string line;
    // read the first line (cpu header)
    if (!std::getline(file, line)) {
        std::cerr << "Empty file or failed to read the first line." << std::endl;
        file.close();
        return;
    }

    // calculate the width of the middle part (cpu irq)
    size_t descStart = line.find_last_not_of(' ');
    if (descStart == std::string::npos) {
        std::cerr << "Invalid format in the first line." << std::endl;
        file.close();
        return;
    }
    descStart++;

    while (std::getline(file, line)) {
        if (line.empty()) {
            break;
        }
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        int irqNum = 0;
        try {
            irqNum = std::stoi(line.substr(0, colonPos));
        } catch (...) {
            continue;
        }

        if (descStart >= line.size()) {
            continue; // not have desc
        }
        // delete space and middle cpu hirq
        std::string desc = line.substr(descStart);
        size_t firstNonSpace = desc.find_first_not_of(" \t");
        size_t lastNonSpace = desc.find_last_not_of(" \t");
        if (firstNonSpace != std::string::npos && lastNonSpace != std::string::npos) {
            desc = desc.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
        }
        irqWithDesc[irqNum] = desc;
    }
    file.close();
}

// make sure net interface info init complete before use this function
void EthQueInfo::AddRegex(const std::vector<std::string> &regexSrc)
{
    for (const auto &str : regexSrc) {
        std::string regexTmp = oeaware::ReplaceString(str, "VAR_BUSINFO", busInfo);
        regexTmp = oeaware::ReplaceString(regexTmp, "VAR_ETH", dev);
        regexTmp = oeaware::ReplaceString(regexTmp, "VAR_DRIVER", driver);
        regexTmp = oeaware::ReplaceString(regexTmp, "VAR_QUEUE", "(\\d+)");
        /* todo
        * 1. replace special symbols in VAR_XXX to avoid regex error
        * 2. use VAR_ETH_BEGIN-eth0-VAR_ETH_END to support special eth name
        */
        regex.emplace_back(regexTmp);
    }
}

void EthQueInfo::UpdateQue2Irq(std::unordered_map<int, std::string> &irqWithDesc)
{
    auto it = regex.begin();
    while (it != regex.end()) {
        std::string re = *it;
        // todo: add debug trace
        try {
            std::regex pattern(re);
            for (const auto &irqIt : irqWithDesc) {
                const int &irqId = irqIt.first;
                const std::string &desc = irqIt.second;
                std::smatch matches;
                if (!std::regex_search(desc, matches, pattern) || matches.size() <= 1) {
                    continue;
                }
                try {
                    // to do : add debug trace
                    que2Irq[std::stoi(matches[1])] = irqId;
                }
                catch (...) {
                    continue;
                }
            }
        }
        catch (...) {
            // todo: add debug trace
            it = regex.erase(it);
        }
        ++it;
    }
}
void EthQueInfo::ProcSpecialEth()
{
    /*
     * drivers\net\ethernet\hisilicon\hns3\hns3_enet.c
     * ref func : hns3_nic_init_irq
     */
    if (driver == "hns3") {
        std::unordered_map<int, int> que2IrqNew;
        for (auto &item : que2Irq) {
            // 2 : description is twice the actual queue number
            que2IrqNew[item.first / 2] = item.second;
        }
        que2Irq = que2IrqNew;
    }
}
bool IrqFrontEnd::InitConf(const std::string &path)
{
    confPath = path;
    return ReadQue2IrqConf();
}

bool IrqFrontEnd::ReadQue2IrqConf()
{
    std::ifstream configFile(confPath);
    if (!configFile.is_open()) {
        return false;
    }

    std::string line;
    bool inQueue2IrqSection = false;
    const std::string prefix = "REGEXP:";

    while (std::getline(configFile, line)) {
        // Trim leading and trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

        if (line.empty() || line[0] == '#') {
            continue; // Skip empty lines and comments
        }

        if (line == "[Queue2Irq]") {
            inQueue2IrqSection = true;
            continue;
        }
        if (!inQueue2IrqSection) {
            continue;
        }
        if (line.find("[") == 0) {
            break; // End of [Queue2Irq] section
        }
        if (line.find(prefix) == 0) {
            std::string pattern = line.substr(prefix.length()); // Remove "REGEXP:" prefix
            if (!pattern.empty()) {
                queRegex.emplace_back(pattern);
            }
        }
    }
    configFile.close();
    return true;
}

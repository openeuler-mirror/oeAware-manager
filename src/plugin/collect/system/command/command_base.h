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

#ifndef COMMAND_BASE_H
#define COMMAND_BASE_H
#include <vector>
#include <unordered_map>
#include <securec.h>
#include <mutex>
#include <atomic>
#include "oeaware/topic.h"

class CommandBase {
public:
    std::mutex dataMutex;
    std::atomic<bool> isRunning{false};
    std::atomic<bool> hasNewData{false};
    // key: attribute row type(topic type + attrsFirst), value: attribute row.
    std::unordered_map<std::string, std::vector<std::string>> dataTypes;
    std::unordered_map<std::string, std::vector<std::vector<std::string>>> datas;
    std::vector<std::string> names;
    // Current attribute row.
    std::string nowType;
    oeaware::Topic topic;
    std::unordered_map<std::string, std::vector<std::string>> attrsFirst;
    std::vector<std::string> skipLine{"---swap--", "Average:"};
    static std::vector<std::string> command;
    static std::vector<std::string> illegal;
    CommandBase();
    virtual ~CommandBase() = default;
    static bool ValidateArgs(const oeaware::Topic& topic);
    static bool ValidateCmd(const std::string &cmd);
    void ParseLine(const std::string& line);
    static std::string GetCommand(const oeaware::Topic& topic);
    bool FillDataStruct(void* dataStruct);
    void Close();
};

class PopenProcess {
public:
    int Pclose();
    void Popen(const std::string &cmd);
    
    FILE *stream;
    pid_t pid;
};

#endif

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

#include <unordered_map>
#include <securec.h>
#include "data_list.h"
#include "interface.h"

class CommandBase {
public:
    std::mutex dataMutex;
    std::atomic<bool> isRunning{false};
    std::atomic<bool> hasNewData{false};
    std::unordered_map<std::string, double> data;

    virtual ~CommandBase() = default;
    virtual bool ValidateArgs(const std::string& args) = 0;
    virtual void ParseLine(const std::string& line) = 0;
    virtual std::string GetCommand(const std::string& params) = 0;
    virtual void* CreateDataStruct() = 0;
    virtual bool FillDataStruct(void* dataStruct) = 0;
};

#endif

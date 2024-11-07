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

#ifndef MPSTAT_COLLECTOR_H
#define MPSTAT_COLLECTOR_H

#include "command_base.h"
#include "command_data.h"

class MpstatCollector : public CommandBase {
protected:
    bool ValidateArgs(const std::string& args) override;
    void ParseLine(const std::string& line) override;
    std::string GetCommand(const std::string& params) override;
    void* CreateDataStruct() override;
    bool FillDataStruct(void* dataStruct) override;
private:
    std::vector<std::string> names;
};

#endif

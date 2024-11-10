/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef STEALTASK_TUNE_H
#define STEALTASK_TUNE_H

#include "interface.h"

namespace oeaware {
class StealTask : public Interface{
public:
    StealTask();
    ~StealTask() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    bool isInit = false;
    const std::string CMDLINE_PATH = "/proc/cmdline";
    std::string cmdline {};
    void ReadConfig();
};
}

#endif // !STEALTASK_TUNE_H

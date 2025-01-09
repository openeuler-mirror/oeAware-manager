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

#ifndef OEAWARE_MANAGER_SMC_TUNE_H
#define OEAWARE_MANAGER_SMC_TUNE_H
#include "oeaware/interface.h"

namespace oeaware {
class SmcTune : public Interface {
public:
    SmcTune();
    ~SmcTune() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    std::pair<std::string, std::string> ReadConfig(const std::string &path);
    std::string blackPortList;
    std::string whitePortList;
};
} // namespace oeaware
#endif // OEAWARE_MANAGER_SMC_TUNE_H

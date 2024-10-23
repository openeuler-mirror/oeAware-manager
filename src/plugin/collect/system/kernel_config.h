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

#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H
#include <unordered_map>
#include "data_list.h"
#include "interface.h"
#include "kernel_data.h"

class KernelConfig : public oeaware::Interface {
public:
    KernelConfig();
    ~KernelConfig() override = default;
    int OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const oeaware::DataList &dataList) override;
    int Enable(const std::string &parma = "") override;
    void Disable() override;
    void Run() override;
private:
    std::vector<std::string> topicStr = {"get_kernel_config", "set_kernel_config"};
    std::vector<std::string> kernelInfo;
    std::string params;
    void searchRegex(const std::string &regexStr, std::shared_ptr <KernelData> &data);
};

#endif

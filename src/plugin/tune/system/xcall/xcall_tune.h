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
#ifndef XCALL_TUNE_H
#define XCALL_TUNE_H
#include "interface.h"
#include <unordered_map>
#include <vector>
#include "data_list.h"
#include "thread_info.h"

class XcallTune : public oeaware::Interface {
public:
    XcallTune();
    ~XcallTune() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
private:
    int ReadConfig(const std::string &path);
    const int defaultPeriod = 1000;
    const int defaultPriority = 2;
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> xcallTune;
    std::unordered_map<std::string, int> threadId;
};

#endif

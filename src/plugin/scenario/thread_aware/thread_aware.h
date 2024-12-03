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
#ifndef THREAD_AWARE_H
#define THREAD_AWARE_H
#include "oeaware/interface.h"
#include "oeaware/data/thread_info.h"

namespace oeaware {
class ThreadAware : public Interface {
public:
    ThreadAware();
    ~ThreadAware() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
private:
    bool ReadKeyList(const std::string &fileName);

    const int awarePeriod{1000};
    const std::string configPath{"/usr/lib64/oeAware-plugin/thread_scenario.conf"};
    std::vector<ThreadInfo> threadWhite;
    std::vector<ThreadInfo> tmpData;
    std::vector<std::string> keyList;
};
}

#endif // !THREAD_AWARE_H

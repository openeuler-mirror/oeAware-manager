/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef PMU_L3C_COLLECTOR_H
#define PMU_L3C_COLLECTOR_H
#include <unordered_map>
#include <chrono>
#include "oeaware/interface.h"

class PmuL3cCollector : public oeaware::Interface {
public:
    PmuL3cCollector();
    ~PmuL3cCollector() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param = "") override;
    void Disable() override;
    void Run() override;
private:
    int pmuId;
    std::string topicStr = "l3c";
    std::vector<std::string> eventStr;
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
    const std::string l3cPath = "/sys/devices/hisi_sccl1_l3c0";
    int OpenL3c();
};

#endif

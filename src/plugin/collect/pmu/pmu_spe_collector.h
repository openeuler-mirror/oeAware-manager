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

#ifndef PMU_SPE_COLLECTOR_H
#define PMU_SPE_COLLECTOR_H
#include <unordered_map>
#include <chrono>
#include "oeaware/interface.h"

class PmuSpeCollector : public oeaware::Interface {
public:
    PmuSpeCollector();
    ~PmuSpeCollector() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param = "") override;
    void Disable() override;
    void Run() override;
private:
    void DynamicAdjustPeriod(int interval);
    void InitSpeAttr(struct PmuAttr &attr);
    int OpenSpe();

    int pmuId;
    int attrPeriod = 2048;
    std::string topicStr = "spe";
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
    const int timeout = 50;
    const int periodThreshold = 2;
    const int minAttrPeriod = 2048;
    const int maxAttrPeriod = 2048000;
    int readTimeMs = 0; // last period PmuRead() time
};

#endif
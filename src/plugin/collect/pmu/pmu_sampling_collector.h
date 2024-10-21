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

#ifndef PMU_SAMPLING_COLLECTOR_H
#define PMU_SAMPLING_COLLECTOR_H
#include <unordered_map>
#include <chrono>
#include "data_list.h"
#include "interface.h"

class PmuSamplingCollector : public oeaware::Interface {
public:
    PmuSamplingCollector();
    ~PmuSamplingCollector() override = default;
    int OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const oeaware::DataList &dataList) override;
    int Enable(const std::string &parma = "") override;
    void Disable() override;
    void Run() override;
private:
    std::unordered_map<std::string, int> pmuId;
    std::vector<std::string> topicStr = {"cycles"};
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
    void InitSamplingAttr(struct PmuAttr &attr);
    int OpenSampling(const oeaware::Topic &topic);
};

#endif

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

#ifndef PMU_COUNTING_COLLECTOR_H
#define PMU_COUNTING_COLLECTOR_H
#include <unordered_map>
#include <chrono>
#include "data_list.h"
#include "interface.h"

class PmuCountingCollector : public oeaware::Interface {
public:
    PmuCountingCollector();
    ~PmuCountingCollector() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param = "") override;
    void Disable() override;
    void Run() override;
private:
    std::unordered_map<std::string, int> pmuId;
    std::vector<std::string> topicStr = {"cycles", "net:netif_rx", "L1-dcache-load-misses", "L1-dcache-loads",
        "L1-icache-load-misses", "L1-icache-loads", "branch-load-misses", "branch-loads", "dTLB-load-misses",
        "dTLB-loads", "iTLB-load-misses", "iTLB-loads", "cache-references", "cache-misses", "l2d_tlb_refill",
        "l2d_cache_refill", "l1d_tlb_refill", "l1d_cache_refill", "inst_retired", "instructions"};
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
    void InitCountingAttr(struct PmuAttr &attr);
    int OpenCounting(const oeaware::Topic &topic);
};

#endif

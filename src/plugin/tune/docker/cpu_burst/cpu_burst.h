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
#ifndef __CPU_BURST_H__
#define __CPU_BURST_H__

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "oeaware/data_list.h"
#include "oeaware/data/docker_data.h"


struct ContainerTune {
    std::string id = "null";
    int64_t cfs_period_us = 0;
    int64_t cfs_quota_us = 0;
    int64_t cfs_burst_us = 0;
    int64_t cfs_burst_us_ori = 0;
    void Exit();
};

class CpuBurst {
public:
    CpuBurst() {}
    ~CpuBurst() {}
    static CpuBurst &GetInstance()
    {
        static CpuBurst instance;
        return instance;
    }
    CpuBurst(const CpuBurst &) = delete;
    CpuBurst &operator=(const CpuBurst &) = delete;
    bool Init();
    void Update(const DataList &dataList);
    void Tune(int periodMs);
    void Exit();

private:
    unsigned int cpuNum = 0;
    uint64_t maxSysCycles = 0;  // per second
    uint64_t curSysCycles = 0;
    uint64_t maxCpuFreqByDmi;
    std::unordered_map<std::string, ContainerTune> containers;
    void UpdatePmu(const DataList &dataList);
    void UpdateDocker(const DataList &dataList);
};
#endif
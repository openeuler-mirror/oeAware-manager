/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/

#ifndef OEAWARE_MANAGER_ENV_INFO_H
#define OEAWARE_MANAGER_ENV_INFO_H
#include <unordered_map>
#include "oeaware/interface.h"
#include "oeaware/data/env_data.h"

class EnvInfo : public oeaware::Interface {
public:
    EnvInfo();
    ~EnvInfo() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param = "") override;
    void Disable() override;
    void Run() override;
private:
    struct TopicParam {
        bool open = false;
    };
    std::vector<std::string> topicStr = { "static", "realtime", "cpu_util" };
    std::unordered_map<std::string, TopicParam> topicParams;
    EnvStaticInfo envStaticInfo = {};
    EnvRealTimeInfo envRealTimeInfo = {};
    std::vector<std::vector<uint64_t>> cpuTime; // [cpu][type]
    EnvCpuUtilParam envCpuUtilInfo = {};
    void InitNumaDistance();
    bool InitEnvStaticInfo();
    bool InitEnvRealTimeInfo();
    void InitEnvCpuUtilInfo();
    void GetEnvRealtimeInfo();
    void GetEnvCpuUtilInfo();
    bool UpdateProcStat();
    bool UpdateCpuDiffTime(std::vector<std::vector<uint64_t>> oldCpuTime);
    void ResetEnvCpuUtilInfo();
};

#endif // OEAWARE_MANAGER_ENV_INFO_H


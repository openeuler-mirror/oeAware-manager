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
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <string>
#include <iostream>
#include "cpu_burst.h"
#include "cpu_burst_adapt.h"

constexpr int PERIOD = 1000;
constexpr int PRIORITY = 2;

CpuBurstAdapt::CpuBurstAdapt()
{
    name = OE_DOCKER_CPU_BURST_TUNE;
    description = "";
    version = "1.0.0";
    period = PERIOD;
    priority = PRIORITY;
    
    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = this->name;
    supportTopics.push_back(topic);
    subscribeTopics.emplace_back(oeaware::Topic{OE_PMU_COUNTING_COLLECTOR, "cycles", ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_DOCKER_COLLECTOR, OE_DOCKER_COLLECTOR, ""});
}

oeaware::Result CpuBurstAdapt::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void CpuBurstAdapt::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void CpuBurstAdapt::UpdateData(const DataList &dataList)
{
    CpuBurst::GetInstance().Update(dataList);
}

oeaware::Result CpuBurstAdapt::Enable(const std::string &param)
{
    (void)param;
    if (!CpuBurst::GetInstance().Init())
        return oeaware::Result(FAILED, "CpuBurst init failed!");
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }
    return oeaware::Result(OK);
}

void CpuBurstAdapt::Disable()
{
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    CpuBurst::GetInstance().Exit();
}

void CpuBurstAdapt::Run()
{
    CpuBurst::GetInstance().Tune(this->period);
}
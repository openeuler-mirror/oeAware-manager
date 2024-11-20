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
    name = "docker_cpu_burst";
    description = "";
    version = "1.0.0";
    period = PERIOD;
    priority = PRIORITY;
    
    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = this->name;
    supportTopics.push_back(topic);
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
    oeaware::Topic topic;
    topic.instanceName = "pmu_counting_collector";
    topic.topicName = "cycles";
    oeaware::Result ret_pmu = Subscribe(topic);
    topic.instanceName = DOCKER_COLLECTOR;
    topic.topicName = DOCKER_COLLECTOR;
    oeaware::Result ret_docker = Subscribe(topic);
    if (ret_pmu.code != OK || ret_docker.code != OK)
        return oeaware::Result(FAILED, "Subscribe failed!");
    return oeaware::Result(OK);
}

void CpuBurstAdapt::Disable()
{
    CpuBurst::GetInstance().Exit();
}

void CpuBurstAdapt::Run()
{
    CpuBurst::GetInstance().Tune(this->period);
}
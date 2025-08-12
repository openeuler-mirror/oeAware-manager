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
#include "cpu_burst/cpu_burst_adapt.h"
#include "coordination_burst/coordination_burst_adapt.h"
#include "load_based_scheduling/load_based_scheduling.h"
#include "cluster_affinity/cluster_affinity_adapt.h"
using namespace oeaware;

extern "C" void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<CpuBurstAdapt>());
    interface.emplace_back(std::make_shared<CoordinationBurstAdapt>());
    interface.emplace_back(std::make_shared<LoadBasedScheduling>());
    interface.emplace_back(std::make_shared<ClusterAffinityAdapt>());
}
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
#include "hugepage/hugepage_analysis.h"
#include "dynamic_smt/dynamic_smt_analysis.h"
#include "smc_d_scenario/smc_d_analysis.h"
#include "xcall/xcall_analysis.h"
#include "net_hirq/net_hirq_analysis.h"
#include "numa_analysis/numa_analysis.h"
#include "docker_coordination_burst/docker_coordination_burst_analysis.h"
#include "microarch_tidnocmp/microarch_tidnocmp_analysis.h"

extern "C" void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
#ifdef __riscv
    interface.emplace_back(std::make_shared<oeaware::HugePageAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::SmcDAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::DynamicSmtAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::DockerCoordinationBurstAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::NumaAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::NetHirqAnalysis>());
#else
    interface.emplace_back(std::make_shared<oeaware::HugePageAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::DynamicSmtAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::SmcDAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::XcallAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::NetHirqAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::NumaAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::DockerCoordinationBurstAnalysis>());
    interface.emplace_back(std::make_shared<oeaware::MicroarchTidNoCmpAnalysis>());
#endif
}
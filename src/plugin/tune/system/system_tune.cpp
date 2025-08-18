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

#include "oeaware/interface.h"
#include "cpu/stealtask_tune/stealtask_tune.h"
#include "cpu/dynamic_smt_tune/dynamic_smt_tune.h"
#include "cpu/cluster_tune/cluster_cpu_tune.h"
#ifdef BUILD_SMC_TUNE
#include "network/smc_tune/smc_tune.h"
#endif
#include "xcall/xcall_tune.h"
#include "power/seep_tune/seep_tune.h"
#include "transparent_hugepage_tune/transparent_hugepage_tune.h"
#include "cpu/numa_sched_tune/numa_sched_tune.h"
#include "preload/preload_tune.h"
#include "binary/binary_tune.h"
#ifdef BUILD_REALTIME
#include "realtime/realtime_tune.h"
#endif
#ifdef BUILD_NETIRQ_TUNE
#include "network/hardirq_tune/hardirq_tune.h"
#endif
#ifdef BUILD_MULTI_NET_PATH_TUNE
#include "network/multi_net_path/multi_net_path.h"
#endif
using namespace oeaware;

extern "C" void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<StealTask>());
    interface.emplace_back(std::make_shared<DynamicSmtTune>());
    interface.emplace_back(std::make_shared<ClusterCpuTune>());
#ifdef BUILD_SMC_TUNE
    interface.emplace_back(std::make_shared<SmcTune>());
#endif
    interface.emplace_back(std::make_shared<XcallTune>());
    interface.emplace_back(std::make_shared<TransparentHugepageTune>());
    interface.emplace_back(std::make_shared<Seep>());
    interface.emplace_back(std::make_shared<PreloadTune>());
    interface.emplace_back(std::make_shared<BinaryTune>());
    interface.emplace_back(std::make_shared<NumaSchedTune>());
#ifdef BUILD_REALTIME
    interface.emplace_back(std::make_shared<RealTimeTune>());
#endif
#ifdef BUILD_NETIRQ_TUNE
    interface.emplace_back(std::make_shared<NetHardIrq>());
#endif
#ifdef BUILD_MULTI_NET_PATH_TUNE
    interface.emplace_back(std::make_shared<MultiNetPath>());
#endif
}

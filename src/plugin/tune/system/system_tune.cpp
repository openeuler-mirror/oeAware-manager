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

#include "interface.h"
#include "cpu/stealtask_tune/stealtask_tune.h"
#include "network/smc_tune/smc_tune.h"
using namespace oeaware;

extern "C" void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<StealTask>());
    interface.emplace_back(std::make_shared<SmcTune>());
}
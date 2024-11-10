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
#include <stdlib.h>
#include "smc_ueid.h"
#include "smc_tune.h"

#define SMC_OP SmcOperator::getInstance()
using namespace oeaware;
int log_level = 0;
SmcTune::SmcTune()
{
    name = "smc_tune";
    description = "collect information of key thread";
    version = "1.0.0";
    period = -1;
    priority = 2;
    type = 2;
}

oeaware::Result SmcTune::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void SmcTune::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void SmcTune::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

oeaware::Result SmcTune::Enable(const std::string &param)
{
    (void)param;
    int ret = (SMC_OP->enable_smc_acc() == EXIT_SUCCESS ? OK: FAILED);
    return oeaware::Result(ret);
}

void SmcTune::Disable()
{
    if (SMC_OP->disable_smc_acc() == EXIT_FAILURE) {
        WARN(logger, "failed to disable smc acc");
    }
}

void SmcTune::Run()
{

}

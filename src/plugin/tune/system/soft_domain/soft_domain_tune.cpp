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
#include "soft_domain_tune.h"

using namespace oeaware;

SoftDomainTune::SoftDomainTune()
{
    name = "soft_domain_tune";
    description = "Enable SOFT_DOMAIN scheduling feature";
    version = "1.0.0";
    period = defaultPeriod;
    priority = defaultPriority;
    type = TUNE;
}

oeaware::Result SoftDomainTune::OpenTopic(const Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void SoftDomainTune::CloseTopic(const Topic &topic)
{
    (void)topic;
}

void SoftDomainTune::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

oeaware::Result SoftDomainTune::Enable(const std::string &param)
{
    (void)param;
    // TODO: 实现具体功能
    return oeaware::Result(OK, "Soft domain tune enabled");
}

void SoftDomainTune::Disable()
{
    // TODO: 实现具体功能
}

void SoftDomainTune::Run()
{
    // TODO: 实现具体功能
}


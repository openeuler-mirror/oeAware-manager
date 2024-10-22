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

void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<SmcTune>());
}

SmcTune::SmcTune() {
    name = "smc_tune";
    description = "collect information of key thread";
    version = "1.0.0";
    period = -1;
    priority = 2;
    type = 2;
}

int SmcTune::OpenTopic(const oeaware::Topic &topic) {
    (void)topic;
    return 0;
}

void SmcTune::CloseTopic(const oeaware::Topic &topic) {
    (void)topic;
}

void SmcTune::UpdateData(const oeaware::DataList &dataList) {
    (void)dataList;
}

int SmcTune::Enable(const std::string &param) {
    (void)param;
    return SMC_OP->enable_smc_acc() == EXIT_SUCCESS ? 0: -1;
}

void SmcTune::Disable() {
    if (SMC_OP->disable_smc_acc() == EXIT_FAILURE) {
        log_err("failed to disable smc acc\n");
    } else {
        log_info("success to disable smc_acc \n");
    }
}

void SmcTune::Run() {

}
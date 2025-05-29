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
#include "smc_tune.h"
#include <stdlib.h>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include "smc_ueid.h"

#define SMC_OP SmcOperator::getInstance()
using namespace oeaware;
static const std::string SMC_CONFIG_PATH = oeaware::DEFAULT_PLUGIN_CONFIG_PATH + "/smc_acc.yaml";
int log_level = 0;
SmcTune::SmcTune()
{
    name        = OE_SMC_TUNE;
    description = "This solution uses Shared Memory Communications - Direct Memory Access(SMC-D) for TCP"
                  " connections to local peers which also support this function.";
    version     = "1.0.0";
    period      = 1000; // Period in milliseconds
    priority    = 2; // set priority
    type        = TUNE;
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
    if (ReadConfig(SMC_CONFIG_PATH) < 0) {
        return oeaware::Result(FAILED);
    }
    SMC_OP->SetShortConnection(shortConnection);
    SMC_OP->InputPortList(blackPortList, whitePortList);
    int ret = (SMC_OP->EnableSmcAcc() == EXIT_SUCCESS ? OK : FAILED);
    return oeaware::Result(ret);
}

void SmcTune::Disable()
{
    if (SMC_OP->DisableSmcAcc() == EXIT_FAILURE) {
        WARN(logger, "failed to disable smc acc");
    }
}

void SmcTune::Run()
{
    ReadConfig(SMC_CONFIG_PATH);
    if (SMC_OP->IsSamePortList(blackPortList, whitePortList)) {
        return;
    }
    SMC_OP->InputPortList(blackPortList, whitePortList);
    if (SMC_OP->ReRunSmcAcc() != EXIT_SUCCESS)
        WARN(logger, "failed to ReRunSmcAcc");
}

int oeaware::SmcTune::ReadConfig(const std::string &path)
{
    std::ifstream sysFile(path);

    if (!sysFile.is_open()) {
        WARN(logger, "smc_acc.yaml config open failed.");
        return -1;
    }
    YAML::Node node = YAML::LoadFile(path);

    blackPortList = node["black_port_list_param"] ? node["black_port_list_param"].as<std::string>() : "";

    whitePortList = node["white_port_list_param"] ? node["white_port_list_param"].as<std::string>() : "";
    auto s = node["short_connection"] ? node["short_connection"].as<std::string>() : "";
    if (!oeaware::IsNum(s)) {
        WARN(logger, "smc_acc.yaml 'short_connection' error.");
        return -1;
    }
    shortConnection = atoi(s.data());
    if (shortConnection < 0 || shortConnection > 1) {
        WARN(logger, "smc_acc.yaml 'short_connection' value invalid.");
        return -1;
    }
    sysFile.close();
    return 0;
}

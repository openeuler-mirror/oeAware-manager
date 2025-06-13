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
namespace oeaware {
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
    auto ret = ReadConfig(SMC_CONFIG_PATH);
    if (ret.code != OK) {
        return ret;
    }
    SMC_OP->SetShortConnection(shortConnection);
    SMC_OP->InputPortList(blackPortList, whitePortList);
    auto smcRet = (SMC_OP->EnableSmcAcc() == EXIT_SUCCESS ? OK : FAILED);
    if (smcRet == OK) {
        return Result(OK);
    } else {
        return Result(FAILED, "failed to enable smc acc, please check if the kernel module is loaded.");
    }
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

Result SmcTune::ReadConfig(const std::string &path)
{
    if (!FileExist(path)) {
        return Result(FAILED, "smc_acc.yaml config file does not exist, {path:" + path + "}.");
    }
    try {
        YAML::Node node = YAML::LoadFile(path);
        if (!node.IsMap()) {
            return Result(FAILED, "smc_acc.yaml config file format error.");
        }
        for (auto item : node) {
            if (configStrs.find(item.first.as<std::string>()) == configStrs.end()) {
                return Result(FAILED, "smc_acc.yaml config file has unknown key: " + item.first.as<std::string>());
            }
        }
        blackPortList = node["black_port_list_param"] ? node["black_port_list_param"].as<std::string>() : "";
        whitePortList = node["white_port_list_param"] ? node["white_port_list_param"].as<std::string>() : "";
        auto s = node["short_connection"] ? node["short_connection"].as<std::string>() : "";
        if (!oeaware::IsNum(s)) {
            return Result(FAILED, "smc_acc.yaml 'short_connection' error.");
        }
        shortConnection = atoi(s.data());
        if (shortConnection < 0 || shortConnection > 1) {
            return Result(FAILED, "smc_acc.yaml 'short_connection' value invalid.");
        }
    } catch (const YAML::Exception &e) {
        return Result(FAILED, "smc_acc.yaml config parse failed: " + std::string(e.what()));
    }
    return Result(OK);
}
}

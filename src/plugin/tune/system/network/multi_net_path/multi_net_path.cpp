/******************************************************************************
 * Copyright (c) 2025-2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include "multi_net_path.h"
#include <sys/utsname.h>
#include "oeaware/data_list.h"

using namespace oeaware;
MultiNetPath::MultiNetPath()
{
    name = OE_MULTI_NET_PATH_TUNE;
    version = "1.0.0";
    period = 1000;       // 1000 ms
    priority = 2;        // 2: tune instance
    type = TUNE;
    description += "[introduction] \n";
    description += "               The queue of the network interface is compatible with the NUMA \n";
    description += "               where the service is located and is only allocated to this service \n";
}

oeaware::Result MultiNetPath::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void MultiNetPath::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void MultiNetPath::UpdateData(const DataList &dataList)
{
    (void)dataList;
    // todo : low load update net and check
}

oeaware::Result MultiNetPath::Enable(const std::string &param)
{
    bool isActive;
    irqbalanceStatus = false;

    if (!ResolveCmd(param)) {
        return oeaware::Result(FAILED, "param resolve failed\n" + GetHelp());
    }
    if (!CheckParam()) {
        return oeaware::Result(FAILED, "param check failed" + GetHelp());
    }
    // confilct with irqbalance, disable it before insert ko
    if (ServiceIsActive("irqbalance", isActive)) {
        irqbalanceStatus = isActive;
    }
    if (irqbalanceStatus) {
        ServiceControl("irqbalance", "stop");
    }

    if (InsertOeNetCls()) {
        return oeaware::Result(OK);
    }
    // exit and restore irqbalance status
    if (irqbalanceStatus) {
        ServiceControl("irqbalance", "start");
    }
    return oeaware::Result(FAILED, "Insert oenetcls.ko failed");
}

void MultiNetPath::Disable()
{
    int result = system("rmmod oenetcls");
    if (result != 0) {
        ERROR(logger, "rmmod oenetcls failed");
    }
    if (irqbalanceStatus) {
        ServiceControl("irqbalance", "start");
    }
}

void MultiNetPath::Run()
{
}

bool oeaware::MultiNetPath::CheckParam()
{
    bool result = true;
    for (auto &dev : ifname) {
        // need dev support config ntuple
        std::string cmdTmp = "ethtool -k " + dev + " | grep ntuple";
        std::string output;
        if (!ExecCommand(cmdTmp, output) || output.find("ntuple") == std::string::npos
            || output.find("fixed") != std::string::npos) {
            result = false;
            ERROR(logger, "cmdTmp " << cmdTmp << " output" << output);
            ERROR(logger, "dev " << dev << " not support ntuple");
        }
    }
    return result;
}

std::string oeaware::MultiNetPath::GetHelp()
{

    std::string output;
    output += "Usage : oeawarectl -e " + name + " [options] \n";
    for (const auto &item : cmdHelp) {
        output +=  item.second + " \n";
    }
    return output;
}

bool oeaware::MultiNetPath::InsertOeNetCls()
{
    int result = 0;
    std::ifstream file("/proc/modules");
    if (!file) {
        ERROR(logger, "failed to open file: /proc/modules");
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(oenetcls) != std::string::npos) {
            result = -1;
            break;
        }
    }
    if (result != 0) {
        ERROR(logger, "failed to modprobe because oenetcls is existed");
        return false;
    }

    const std::string modCmd = "modprobe oenetcls" + cmd;
    result = system(modCmd.c_str());
    if (result != 0) {
        ERROR(logger, modCmd << " failed, return code is " << result);
        return false;
    }
    INFO(logger, modCmd << " success");

    return true;
}

std::vector<std::string> SplitStringByHash(const std::string &input)
{
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, '#')) {
        result.push_back(token);
    }
    return result;
}

bool oeaware::MultiNetPath::CheckLegal(const std::string &cmd)
{
    std::vector<std::string> illegal = { "|", ";", "&", ">", "<",
        "`", "\n", "\"", "\\", "*", "?", "~", "!", "^", "$" };
    for (auto &c : illegal) {
        if (cmd.find(c) != std::string::npos) {
            ERROR(logger, cmd << " illegal character: " << c);
            return false;
        }
    }
    return true;
}

bool MultiNetPath::ResolveCmd(const std::string &param)
{
    if (param.empty()) {
        ERROR(logger, name << " param is empty");
        return false;
    }
    auto paramsMap = GetKeyValueFromString(param);
    for (auto &item : paramsMap) {
        if (!cmdHelp.count(item.first) || !CheckLegal(item.second)) {
            ERROR(logger, "invalid param: " << item.first);
            return false;
        }
    }
    if (paramsMap.count("ifname")) {
        ifnamePara = paramsMap["ifname"].data();
        ifname = SplitStringByHash(ifnamePara);
    }
    if (ifname.empty()) {
        ERROR(logger, "please config ifname");
        return false;
    }

    if (paramsMap.count("appname")) {
        appname = paramsMap["appname"].data();
    } else {
        WARN(logger, "appname is not config, will use default appname(redis-server)");
    }

    cmd = " ifname=" + ifnamePara;
    if (!appname.empty()) {
        if (appname == "all_app") {
            cmd += " appname=\"\"";
        } else {
            cmd += " appname=" + appname;
        }
    }

    if (matchIp != invaildParam) {
        cmd += " match_ip_flag=" + std::to_string(matchIp);
    }

    if (mode != invaildParam) {
        cmd += " mode=" + std::to_string(mode);
    }
    return true;
}


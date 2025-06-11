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
#include "xcall_tune.h"
#include <iostream>
#include <numa.h>
#include <yaml-cpp/yaml.h>
#include "oeaware/data/thread_info.h"
#include "oeaware/utils.h"


static const std::string XCALL_CONFIG_PATH = oeaware::DEFAULT_PLUGIN_CONFIG_PATH + "/xcall.yaml";
XcallTune::XcallTune()
{
    name = OE_XCALL_TUNE;
    description = "collect information of key thread";
    version = "1.0.0";
    period = defaultPeriod;
    priority = defaultPriority;
    type = oeaware::TUNE;
}

oeaware::Result XcallTune::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void XcallTune::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void XcallTune::UpdateData(const DataList &dataList)
{
    threadId.clear();
    for (uint64_t i = 0; i < dataList.len; ++i) {
        ThreadInfo *threadInfo = (ThreadInfo*)dataList.data[i];
        if (xcallTune.count(threadInfo->name)) {
            threadId[threadInfo->name].insert(threadInfo->pid);
        }
    }
}

oeaware::Result XcallTune::ReadConfig(const std::string &path)
{
    std::ifstream sysFile(path);
    if (!sysFile.is_open()) {
        return oeaware::Result(FAILED, "xcall config open failed.");
    }
    try {
        YAML::Node node = YAML::LoadFile(path);
        if (!node.IsMap()) {
            return oeaware::Result(FAILED, "xcall config format error.");
        }
        for (auto item : node) {
            std::string threadName = item.first.as<std::string>();
            if (!item.second.IsSequence()) {
                return oeaware::Result(FAILED, "xcall config('" + threadName + "') error.");
            }
            for (auto it : item.second) {
                if (!it.IsMap()) {
                    return oeaware::Result(FAILED, "xcall config('" + threadName + "') error.");
                }
                for (auto p : it) {
                    std::string xcallName = p.first.as<std::string>();
                    std::string xcallEvent = p.second.as<std::string>();
                    if (!xcallType.count(xcallName)) {
                        return oeaware::Result(FAILED, "xcall config('" + xcallName + "') error.");
                    }
                    auto events = oeaware::SplitString(xcallEvent, ",");
                    for (auto &event: events) {
                        xcallTune[threadName].emplace_back(std::make_pair(xcallName, event));
                    }
                }
            }
        }
    } catch (const YAML::Exception &e) {
        return oeaware::Result(FAILED, "xcall config parse failed, " + std::string(e.what()));
    }
    return oeaware::Result(OK);
}

static std::string ReadCpuList(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::string content;
    std::getline(file, content);
    return content;
}

static std::vector<int> GetNumaCpuRange(int i)
{
    std::string path = "/sys/devices/system/node/node" + std::to_string(i) + "/cpulist";
    auto cpuList = ReadCpuList(path);
    std::istringstream iss(cpuList);
    std::string range;
    std::getline(iss, range);
    return oeaware::ParseRange(range);
}

void XcallTune::EnablePrefetchCpu()
{
    // Now only support 4 numas
    int sysNumaNum = numa_num_configured_nodes();
    int supportNumaNum = 4;
    for (int i = 0; i < supportNumaNum && i < sysNumaNum; ++i) {
        auto p = GetNumaCpuRange(i);
        if (!p.empty()) {
            // Each numa uses one core for prefetching.
            WriteSysParam("/proc/sys/kernel/xcall_numa" + std::to_string(i) + "_cpumask", std::to_string(p[0]));
        }
    }
}

oeaware::Result XcallTune::Enable(const std::string &param)
{
    if (!oeaware::FileExist("/proc/1/xcall")) {
        return oeaware::Result(FAILED, "xcall does not open. If the system supports xcall, "
                "please add 'xcall' to cmdline.");
    }
    auto params = oeaware::GetKeyValueFromString(param);
    std::string invalid = "";
    for (auto &p : params) {
        if (p.first == "c") {
            continue;
        }
        if (!invalid.empty()) invalid += ",";
        invalid += p.first;
    }
    if (!invalid.empty()) {
        return oeaware::Result(FAILED, "params(" + invalid + ") invalid.");
    }
    if (params.count("c")) {
        configPath = params["c"];
    } else {
        configPath = XCALL_CONFIG_PATH;
    }
    auto ret = ReadConfig(configPath);
    if (ret.code < 0) {
        return ret;
    }
    EnablePrefetchCpu();
    Subscribe(oeaware::Topic{"thread_collector", "thread_collector", ""});
    return oeaware::Result(OK);
}

void XcallTune::Disable()
{
    Unsubscribe(oeaware::Topic{"thread_collector", "thread_collector", ""});
    for (auto &p : openedXcall) {
        if (!oeaware::FileExist(p.first)) {
            continue;
        }
        for (auto &value : p.second) {
            WriteSysParam(p.first, "!"+value);
        }
    }
    openedXcall.clear();
    xcallTune.clear();
    threadId.clear();
}

int XcallTune::WriteSysParam(const std::string &path, const std::string &value)
{
    std::ofstream sysFile(path);
    if (!sysFile.is_open()) {
        return -1;
    }
    sysFile << value;
    if (sysFile.fail()) {
        return -1;
    }
    sysFile.close();
     if (sysFile.fail()) {
        return -1;
    }
    return 0;
}

static bool SyscallNumberRange(const std::string &s)
{
    int number = atoi(s.data());
    for (auto &r : XCALL_RANGE) {
        if (number >= r[0] && number <= r[1]) {
            return true;
        }
    }
    return false;
}

void XcallTune::Run()
{
    for (auto &item : threadId) {
        for (auto &pid : item.second) {
            std::string path = "/proc/" + std::to_string(pid) + "/xcall";
            if (openedXcall.count(path)) {
                continue;
            }
            for (auto v : xcallTune[item.first]) {
                if (!xcallType.count(v.first)) {
                    continue;
                }
                if (!oeaware::IsInteger(v.second)) {
                    WARN(logger, "systcall number must be a integer, but " << v.second);
                    continue;
                }
                if (!SyscallNumberRange(v.second)) {
                    WARN(logger, "syscall number range [0, 294], [403, 469], but " << v.second);
                    continue;
                }
                if (v.first == "xcall_1") {
                    if (WriteSysParam(path, v.second) == 0) {
                        openedXcall[path].emplace_back(v.second);
                        INFO(logger, "xcall applied, {path: " << path << ", type: xcall_1, value: " << v.second <<
                                "}.");
                    } else {
                        WARN(logger, "xcall applied failed, {path: " << path << ", type: xcall_2, value: " <<
                                v.second << "}.");
                    }
                } else if (v.first == "xcall_2") {
                    std::string value = "@" + v.second;
                    if (WriteSysParam(path, value) == 0) {
                        openedXcall[path].emplace_back(v.second);
                        INFO(logger, "xcall pretetch applied, {path: " << path << ", type: xcall_2, value: " <<
                            v.second << "}.");
                    } else {
                        WARN(logger, "xcall pretetch applied failed, path: {" << path << ", type: xcall_2, value: " <<
                            v.second << "}.");
                    }
                }
            }
        }
    }
}
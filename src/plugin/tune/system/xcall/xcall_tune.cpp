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
#include <yaml-cpp/yaml.h>
#include "oeaware/data/thread_info.h"
#include "oeaware/utils.h"

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

int XcallTune::ReadConfig(const std::string &path)
{
    std::ifstream sysFile(path);
    if (!sysFile.is_open()) {
        WARN(logger, "xcall config open failed.");
        return -1;
    }
    YAML::Node node = YAML::LoadFile(path);
    if (!node.IsMap()) {
        return -1;
    }
    for (auto item : node) {
        std::string threadName = item.first.as<std::string>();
        for (auto item : item.second) {
            for (auto p : item) {
                std::string xcallName = p.first.as<std::string>();
                std::string xcallEvent = p.second.as<std::string>();
                if (xcallName != "xcall_1") {
                    WARN(logger, "xcall config('" << xcallName << "') error.");
                    return -1;
                }
                auto events = oeaware::SplitString(xcallEvent, ",");
                for (auto &event: events) {
                    xcallTune[threadName].emplace_back(std::make_pair(xcallName, event));
                }
            }
        }
    }
    sysFile.close();
    return 0;
}

oeaware::Result XcallTune::Enable(const std::string &param)
{
    (void)param;
    if (!oeaware::FileExist("/proc/1/xcall")) {
        return oeaware::Result(FAILED, "xcall does not open. If the system supports xcall, \
                please add 'xcall' to cmdline.");
    }
    auto params = oeaware::GetKeyValueFromString(param);
    if (!params.empty()) {
        if (params.count("c")) {
            configPath = params["c"];
        } else {
            std::string invalid = "";
            for (auto &p : params) {
                if (!invalid.empty()) invalid += ",";
                invalid += p.first;
            }
            return oeaware::Result(FAILED, "params(" + invalid + ") invalid");
        }
    } else {
        configPath = "/usr/lib64/oeAware-plugin/xcall.yaml";
    }
    int ret = ReadConfig(configPath);
    Subscribe(oeaware::Topic{"thread_collector", "thread_collector", ""});
    return oeaware::Result(ret);
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
                if (v.first != "xcall_1") {
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
                if (WriteSysParam(path, v.second) == 0) {
                    openedXcall[path].emplace_back(v.second);
                    INFO(logger, "xcall applied, {path: " << path << ", type: xcall_1, value: " << v.second <<
                            "}.");
                } else {
                    WARN(logger, "xcall applied failed, path: " << path << ".");
                }
            }
        }
    }
}
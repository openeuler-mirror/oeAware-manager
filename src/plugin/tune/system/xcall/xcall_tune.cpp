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

XcallTune::XcallTune()
{
    name = OE_XCALL_TUNE;
    description = "collect information of key thread";
    version = "1.0.0";
    period = defaultPeriod;
    priority = defaultPriority;
    type = oeaware::TUNE | oeaware::RUN_ONCE;
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
    for (uint64_t i = 0; i < dataList.len; ++i) {
        ThreadInfo *threadInfo = (ThreadInfo*)dataList.data[i];
        if (xcallTune.count(threadInfo->name)) {
            threadId[threadInfo->name] = threadInfo->pid;
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
                xcallTune[threadName].emplace_back(std::make_pair(xcallName, xcallEvent));
            }
        }
    }
    sysFile.close();
    return 0;
}

oeaware::Result XcallTune::Enable(const std::string &param)
{
    (void)param;
    int ret = ReadConfig("/usr/lib64/oeAware-plugin/xcall.yaml");
    Subscribe(oeaware::Topic{"thread_scenario", "thread_scenario", ""});
    return oeaware::Result(ret);
}

void XcallTune::Disable()
{
    Unsubscribe(oeaware::Topic{"thread_scenario", "thread_scenario", ""});
}

static int WriteSysParam(const std::string &path, const std::string &value)
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

void XcallTune::Run()
{
    for (auto &p : threadId) {
        std::string path = "/proc/" + std::to_string(p.second) + "/xcall";
        for (auto v : xcallTune[p.first]) {
            if (v.first != "xcall_1") {
                continue;
            }
            if (WriteSysParam(path, v.second) == 0) {
                INFO(logger, "xcall applied, path: " << path << ".");
            } else {
                WARN(logger, "xcall applied failed, path: " << path << ".");
            }
        }
    }
}
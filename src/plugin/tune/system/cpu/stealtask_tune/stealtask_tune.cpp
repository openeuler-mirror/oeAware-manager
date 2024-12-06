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

#include "stealtask_tune.h"
#include <fstream>

using namespace oeaware;

StealTask::StealTask()
{
    name = OE_STEALTASK_TUNE;
    version = "1.0.0";
    period = 1000; // 1000 period is meaningless, only Enable() is executed, not Run()
    priority = 2;
    type = TUNE;
    description = "steal task is a lightweight scheduling feature that improves CPU \
    utilization by increasing the success rate of idle CPUs actively pulling tasks";
}

oeaware::Result StealTask::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void StealTask::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void StealTask::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

oeaware::Result StealTask::Enable(const std::string &param)
{
    (void)param;
    if (system("zcat /proc/config.gz | grep CONFIG_SCHED_STEAL=y > /dev/null") == 1) {
        return oeaware::Result(FAILED, "kernel should be compiled with CONFIG_SCHED_STEAL=y");
    }

    // some kernels enable stealtask doesn't need configuring cmdline, example openEuler 5.10 kernel update
    // other kernels may need configuring cmdline, example openEuler 6.6 kernel
    // ref : https://gitee.com/openeuler/kernel/issues/IAQWPQ
    ReadConfig();
    std::string::size_type pos = cmdline.find("sched_steal_node_limit");
    if (pos == std::string::npos) {
        WARN(logger, "sched_steal_node_limit=[numa number] not found in cmdline");
        WARN(logger, "enabling stealtask in certain kernels requires configuring CMDline, "
            << "which may not be effective if not configured");
    }

    if (!InitSchedFeaturesPath()) {
        return oeaware::Result(FAILED, "Failed to found sched features file");
    }

    if (!SetSchedFeatures("STEAL")) {
        return oeaware::Result(FAILED, "Failed to set sched features");
    }
    return oeaware::Result(OK);
}

void StealTask::Disable()
{
    SetSchedFeatures("NO_STEAL");
}

void StealTask::Run()
{
}

void StealTask::ReadConfig()
{
    std::ifstream file(CMDLINE_PATH);
    if (!file.is_open()) {
        return;
    }

    std::getline(file, cmdline);
    file.close();
}

bool oeaware::StealTask::InitSchedFeaturesPath()
{
    if (schedFeaturesPath != "") {
        return true;
    }
    // different kernel version has different sched features path
    std::vector<std::string> paths = {
        "/sys/kernel/debug/sched/features",
    "/sys/kernel/debug/sched_features"};
    std::string logTmp = "";
    for (const auto &path : paths) {
        logTmp += path;
        logTmp += " ";
        std::ofstream file(path, std::ios::in);
        if (file) {
            schedFeaturesPath = path;
            file.close();
            return true;
        }
    }
    schedFeaturesPath = "";
    ERROR(logger, "StealTask failed open " << logTmp);
    return false;
}

bool oeaware::StealTask::SetSchedFeatures(const std::string &schedFeatures)
{
    std::ofstream file(schedFeaturesPath, std::ios::out | std::ios::in);
    if (!file.is_open()) {
        WARN(logger, "Failed to open file: " << schedFeaturesPath);
        return false;
    }
    file << schedFeatures;
    if (!file.flush()) {
        WARN(logger, "Failed to flush << " << schedFeatures << " to file: " << schedFeaturesPath);
        return false;
    }
    file.close();
    INFO(logger, "Successfully set " << schedFeaturesPath << " to: " << schedFeatures);
    return true;
}

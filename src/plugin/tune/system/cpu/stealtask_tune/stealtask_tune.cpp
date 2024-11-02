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
#include "interface.h"
#include <fstream>

using namespace oeaware;

StealTask::StealTask() {
    name = "stealtask_tune";
    description = "";
    version = "1.0.0";
    period = -1;
    priority = 2;
    type = 1;

}

int StealTask::OpenTopic(const oeaware::Topic &topic) {
    (void)topic;
    return true;
}

void StealTask::CloseTopic(const oeaware::Topic &topic) {
    (void)topic;
}

void StealTask::UpdateData(const oeaware::DataList &dataList) {
    (void)dataList;
}

int StealTask::Enable(const std::string &param) {
    (void)param;
    if (system("zcat /proc/config.gz | grep CONFIG_SCHED_STEAL=y > /dev/null") == 1) {
        return false;
    }

    if (!isInit) {
        ReadConfig();
        std::ofstream file("/sys/kernel/debug/sched_features");
        file << "STEAL";
        file.close();
        isInit = true;
    }

    std::string::size_type pos = cmdline.find("sched_steal_node_limit");
    if (pos == std::string::npos) {
        return false;
    }

    return true;
}

void StealTask::Disable() {
    std::ofstream file("/sys/kernel/debug/sched_features");
    file << "NO_STEAL";
    file.close();
    isInit = false;
}

void StealTask::Run() {

}

void StealTask::ReadConfig() {
    std::ifstream file(CMDLINE_PATH);
    if (!file.is_open()) {
        return;
    }

    std::getline(file, cmdline);
    file.close();
}
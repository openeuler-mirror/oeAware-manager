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

#include "seep_tune.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unistd.h>

using namespace oeaware;

Seep::Seep()
{
    name = OE_SEEP_TUNE;
    description = "seep governor is specifically designed for platforms with hardware-manager P-stats through CPPC";
    version = "1.0.0";
    period = 1000;  // 1000 period is meaningless, only Enable() is executed, not Run()
    priority = 2;
    type = oeaware::TUNE;
}

oeaware::Result Seep::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void Seep::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void Seep::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

static bool IsFileExists(const std::string &path)
{
    return access(path.c_str(), F_OK) == 0;
}

bool Seep::WriteFile(const std::string &filePath, const std::string &value)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    file << value;
    if (!file.flush()) {
        ERROR(logger, "Failed to echo " << value << " to " << filePath);
        return false;
    }
    file.close();
    return true;
}

bool Seep::WriteSeepFile(const unsigned int &cpu, const std::string &value)
{
    std::string seepPath = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cpufreq/scaling_governor";
    if (!IsFileExists(seepPath)) {
        ERROR(logger, "cpu" << cpu << " seep directory not available");
        return false;
    }

    DEBUG(logger, "Writing " << value << " to: " << seepPath);
    if (WriteFile(seepPath, value)) {
        DEBUG(logger, "Successfully write " << value << " for cpu" << cpu);
        return true;
    } else {
        ERROR(logger, "Failed to write " << value << " for cpu" << cpu);
        return false;
    }
}

bool Seep::Init()
{
    for (int i = 0; i < cpuNum; i++) {
        std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_governor";
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        std::string temp;
        std::getline(file, temp);
        cpuScaling.push_back(temp);
        file.close();
    }
    return true;
}

bool Seep::InsertSeepKo()
{
    // 判断是否已经加载了 cpufreq_seep.ko
    int result = 0;
    std::ifstream file("/proc/modules");
    if (!file) {
        ERROR(logger, "failed to open file: /proc/modules");
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("cpufreq_seep") != std::string::npos) {
            result = -1;
            break;
        }
    }
    if (result != 0) {
        ERROR(logger, "failed to modprobe because cpufreq_seep is existed");
        return false;
    }

    // modprobe cpufreq_seep
    result = system("modprobe cpufreq_seep");
    if (result != 0) {
        ERROR(logger, "modprobe cpufreq_seep failed, please check bios configure: custom mode");
        return false;
    }

    return true;
}

oeaware::Result Seep::Enable(const std::string &param)
{
    (void)param;
    int result;

    // 后续要考虑seep已经使能的情况下插件该如何管理，以及部分cpu offline时的调优
    // openEuler版本默认不加载seep调频模块，需要手动加载
    result = InsertSeepKo();
    if (!result) {
        ERROR(logger, "modprobe cpufreq_seep.ko failed");
        return oeaware::Result(FAILED, "modprobe cpufreq_seep.ko failed");
    }

    cpuNum = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpuNum == -1) {
        ERROR(logger, "can not get the cpu num");
        return oeaware::Result(FAILED, "can not get the cpu num");
    }

    // 获取各cpu的初始状态
    result = Init();
    if (!result) {
        ERROR(logger, "Init failed");
        return oeaware::Result(FAILED, "Init failed");
    }

    // 设置scaling_governor为seep，可通过以下参数设置seep调频器的参数：
    // 1、/sys/devices/system/cpu/cpuX/cpufreq/seep_auto_act_window，默认10，单位ms，表示bios的调频周期
    // 2、/sys/devices/system/cpu/cpuX/cpufreq/seep_energy_perf，默认5，单位%，表示相比performance尽可能让性能损失不超过5%
    for (int i = 0; i < cpuNum; ++i) {
        result = WriteSeepFile(i, "seep");
        if (!result) {
            DEBUG(logger, "Try to restore the enabled cpu");
            for (int j = 0; j < i; ++j) {
                WriteSeepFile(j, cpuScaling[j]);
            }
            return oeaware::Result(FAILED, "write seep to cpu" + std::to_string(i) + " failed");
        }
    }

    return oeaware::Result(OK);
}

void Seep::Disable()
{
    for (unsigned int i = 0; i < cpuNum; ++i) {
        WriteSeepFile(i, cpuScaling[i]);
    }
    cpuScaling.clear();
}

void Seep::Run()
{
}

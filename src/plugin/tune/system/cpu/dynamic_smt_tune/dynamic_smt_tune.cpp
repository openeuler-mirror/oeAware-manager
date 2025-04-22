/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/

#include "dynamic_smt_tune.h"
#include <fstream>

using namespace oeaware;

DynamicSmtTune::DynamicSmtTune()
{
    name = OE_DYNAMICSMT_TUNE;
    version = "1.0.0";
    period = 1000; // 1000 period is meaningless, only Enable() is executed, not Run()
    priority = defaultPriority;
    type = TUNE;
    description = "dynamic smt intelligently allocates computational resources by \
    activating idle physical cores during low system loads to improve performance";
}

oeaware::Result DynamicSmtTune::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void DynamicSmtTune::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void DynamicSmtTune::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

oeaware::Result DynamicSmtTune::Enable(const std::string &param)
{
    if (!param.empty()) {
        auto paramsMap = GetKeyValueFromString(param);
        if (paramsMap.count("threshold")) {
            schedUtilRatio = atof(paramsMap["threshold"].data());
            if (schedUtilRatio < DYNAMIC_SMT_MIN_THRESHOLD || schedUtilRatio > DYNAMIC_SMT_MAX_THRESHOLD) {
                return oeaware::Result(FAILED, "Failed to set schedUtilRatio: " + schedUtilRatio);
            }
        }
    } else {
        schedUtilRatio = defaultRatio;
    }

    if (!CheckEnv()) {
        return oeaware::Result(FAILED, "Please enable SMT and include CONFIG_SCHED_SMT in kernel");
    }

    if (!InitSchedParam()) {
        return oeaware::Result(FAILED, "Failed to found sched features file");
    }

    if (!SetSchedFeatures(schedRatioPath, schedUtilRatio)) {
        return oeaware::Result(FAILED, "Failed to set sched_util_ratio");
    }

    if (!SetSchedFeatures(schedFeaturesPath, "KEEP_ON_CORE")) {
        return oeaware::Result(FAILED, "Failed to set sched features");
    }
    return oeaware::Result(OK);
}

void DynamicSmtTune::Disable()
{
    if (!initialStatus) {
        if (!SetSchedFeatures(schedFeaturesPath, "NO_KEEP_ON_CORE")) {
            ERROR(logger, "Failed to restore initial state of sched features");
        }
    }
    if (initialRatio != -1) {
        if (!SetSchedFeatures(schedRatioPath, initialRatio)) {
            ERROR(logger, "Failed to restore initial state of sched_util_ratio");
        }
    }
}

void DynamicSmtTune::Run()
{
}

bool oeaware::DynamicSmtTune::CheckEnv()
{
    std::ifstream smtFile("/sys/devices/system/cpu/smt/active");
    if (!smtFile.is_open()) {
        ERROR(logger, "Failed to open SMT status file");
        return false;
    }

    int smtActive;
    smtFile >> smtActive;
    if (smtActive != 1) {
        ERROR(logger, "SMT is not active");
        return false;
    }

    FILE* pipe = popen("zcat /proc/config.gz | grep CONFIG_SCHED_SMT", "r");
    if (!pipe) {
        ERROR(logger, "Failed to check kernel configuration");
        return false;
    }

    char buffer[128];
    bool found = false;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        if (strstr(buffer, "CONFIG_SCHED_SMT=y") != nullptr) {
            found = true;
            break;
        }
    }

    pclose(pipe);
    if (!found) {
        ERROR(logger, "CONFIG_SCHED_SMT is not enabled in kernel");
        return false;
    }

    return true;
}

bool oeaware::DynamicSmtTune::InitSchedParam()
{
    // different kernel version has different sched features path
    std::vector<std::string> paths = {
        "/sys/kernel/debug/sched/features",
        "/sys/kernel/debug/sched_features"};
    std::string logTmp = "";
    for (const auto &path : paths) {
        logTmp += path;
        logTmp += " ";
        std::ifstream file(path, std::ios::in);
        if (file) {
            schedFeaturesPath = path;
            std::string line;
            while (std::getline(file, line)) {
                if (line.find("KEEP_ON_CORE") != std::string::npos) {
                    initialStatus = line.find("NO_KEEP_ON_CORE") == std::string::npos;
                    break;
                }
            }
        }
    }
    if (schedFeaturesPath == "") {
        ERROR(logger, "DynamicSmtTune failed open " << logTmp);
        return false;
    }

    std::ifstream ratioFile(schedRatioPath, std::ios::in);
    if (ratioFile) {
        ratioFile >> initialRatio;
        ratioFile.close();
    } else {
        ERROR(logger, "Failed to read initial ratio from " << schedRatioPath);
        return false;
    }

    return true;
}

template<typename T>
bool oeaware::DynamicSmtTune::SetSchedFeatures(const std::string &filePath, const T &value)
{
    std::ofstream file(filePath, std::ios::out | std::ios::in);
    if (!file.is_open()) {
        ERROR(logger, "Failed to open file: " << filePath);
        return false;
    }

    file << value;
    if (!file.flush()) {
        ERROR(logger, "Failed to flush " << value << " to file: " << filePath);
        file.close();
        return false;
    }

    file.close();
    INFO(logger, "Successfully set " << filePath << " to: " << value);
    return true;
}

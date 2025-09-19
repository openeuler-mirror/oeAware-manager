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
#include <iostream>
#include <fstream>
#include "oeaware/utils.h"
#include "numa_sched_tune.h"

using namespace oeaware;

NumaSchedTune::NumaSchedTune()
{
    name = OE_NUMA_SCHED_TUNE;
    description = "To enable NUMA scheduling parallelism feature";
    version = "1.0.0";
    period = 1000; // 1000 period is meaningless, only Enable() is executed, not Run()
    priority = 2; // 2 priority is meaningless
    type = oeaware::TUNE;
    originalSchedUtilLowPct = "";
}

bool NumaSchedTune::IsSupportNumaSched()
{
    for (const auto &feature : schedFeatues) {
        if (feature == enableFeature || feature == disableFeature) {
            return true;
        }
    }
    return false;
}

bool NumaSchedTune::IsFeatureEnabled()
{
    for (const auto &feature : schedFeatues) {
        if (feature == enableFeature) {
            return true;
        }
    }
    return false;
}

bool NumaSchedTune::SaveSchedUtilLowPct()
{
    std::ifstream utilFile(schedUtilLowPctPath);
    if (utilFile.is_open()) {
        std::getline(utilFile, originalSchedUtilLowPct);
        utilFile.close();
        INFO(logger, "[NUMA_SCHED] store origin sched_util_low_pct value: " + originalSchedUtilLowPct);
        return true;
    } else {
        ERROR(logger, "[NUMA_SCHED] Failed to read sched_util_low_pct value");
        return false;
    }
}

bool NumaSchedTune::SetSchedUtilLowPct(const std::string &value)
{
    std::ofstream outUtilFile(schedUtilLowPctPath);
    if (outUtilFile.is_open()) {
        outUtilFile << value;
        outUtilFile.close();
        INFO(logger, "[NUMA_SCHED] set sched_util_low_pct to " + value);
        return true;
    } else {
        ERROR(logger, "[NUMA_SCHED] Failed to set sched_util_low_pct.");
        return false;
    }
}

oeaware::Result NumaSchedTune::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void NumaSchedTune::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void NumaSchedTune::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

oeaware::Result NumaSchedTune::Enable(const std::string &param)
{
    (void)param; // no use param

    schedFeatues.clear();
    oeaware::ReadSchedFeatures(schedFeaturePath, schedFeatues);
    if (!IsSupportNumaSched()) {
        ERROR(logger, "[NUMA_SCHED] not support numa sched feature");
        return oeaware::Result(FAILED, "not support numa sched feature");
    }

    // check if is already enabled on system
    if (IsFeatureEnabled()) {
        INFO(logger, "[NUMA_SCHED] numa sched is already enabled on system");
        isOriginalEnabled = true;
        return oeaware::Result(OK, "numa sched is already enabled on system");
    }

    // save original sched_util_low_pct value if not enabled
    if (!SaveSchedUtilLowPct()) {
        return oeaware::Result(FAILED, "Failed to save sched_util_low_pct");
    }

    // enable numa sched feature
    if (!WriteFeature(enableFeature)) {
        ERROR(logger, "[NUMA_SCHED] Failed to enable numa sched feature");
        return oeaware::Result(FAILED, "Failed to enable numa sched feature");
    }

    // set sched_util_low_pct to 100
    if (!SetSchedUtilLowPct("100")) {
        ERROR(logger, "[NUMA_SCHED] Failed to set sched_util_low_pct to 100");
        (void)WriteFeature(disableFeature); // restore to original mode
        return oeaware::Result(FAILED, "Failed to set ched_util_low_pct to 100");
    }

    INFO(logger, "[NUMA_SCHED] Success to enable numa sched feature");
    return oeaware::Result(OK, "Success to enable numa sched feature");
}

void NumaSchedTune::Disable()
{
    if (isOriginalEnabled) {
        isOriginalEnabled = false;
        return;
    }

    if (WriteFeature(disableFeature)) {
        INFO(logger, "[NUMA_SCHED] Success to restored numa sched mode");
        isOriginalEnabled = false;
    } else {
        ERROR(logger, "[NUMA_SCHED] Failed to restore numa sched mode");
        return;
    }

    if (!originalSchedUtilLowPct.empty()) {
        SetSchedUtilLowPct(originalSchedUtilLowPct);
        originalSchedUtilLowPct = "";
    }
}

void NumaSchedTune::Run()
{
    // Nothing to do in Run() as this is a one-time setting
}

bool NumaSchedTune::WriteFeature(const std::string &feature)
{
    std::ofstream file(schedFeaturePath);
    if (!file.is_open()) {
        ERROR(logger, "[NUMA_SCHED] Failed to open sched_features file: " + schedFeaturePath);
        return false;
    }

    file << feature;
    file.close();

    INFO(logger, "[NUMA_SCHED] Mode set to: " + feature);
    return true;
}
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
#ifndef NUMA_SCHED_TUNE_H
#define NUMA_SCHED_TUNE_H

#include "oeaware/interface.h"

namespace oeaware {

/**
 * @brief NumaSchedTune class for enabling/disabling NUMA scheduling parallelism
 * This class manages the NUMA scheduling parallelism feature by writing
 * to the sched_features kernel interface.
 */
class NumaSchedTune : public Interface {
public:
    NumaSchedTune();
    ~NumaSchedTune() override = default;
    
    // Interface implementation
    Result OpenTopic(const Topic &topic) override;
    void CloseTopic(const Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    // Constants
    const std::string enableFeature = "PARAL";
    const std::string disableFeature = "NO_PARAL";
    const std::string schedUtilLowPctPath = "/proc/sys/kernel/sched_util_low_pct";

    // State tracking
    std::vector<std::string> schedFeatues;
    std::string schedFeaturePath;
    std::string originalSchedUtilLowPct;
    bool isOriginalEnabled = false;
    
    // Helper methods
    bool WriteFeature(const std::string &feature);
    bool IsSupportNumaSched();
    bool IsFeatureEnabled();
    bool SaveSchedUtilLowPct();
    bool SetSchedUtilLowPct(const std::string &value);
};

} // namespace oeaware

#endif // NUMA_SCHED_TUNE_H
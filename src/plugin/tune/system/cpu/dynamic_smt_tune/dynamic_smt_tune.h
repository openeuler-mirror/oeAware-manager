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
#ifndef DYNAMIC_SMT_TUNE_H
#define DYNAMIC_SMT_TUNE_H

#include "oeaware/interface.h"

constexpr int DYNAMIC_SMT_MIN_THRESHOLD = 0;
constexpr int DYNAMIC_SMT_MAX_THRESHOLD = 100;

namespace oeaware {
class DynamicSmtTune : public Interface {
public:
    DynamicSmtTune();
    ~DynamicSmtTune() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    const int defaultPriority = 2;
    bool initialStatus = false;
    int initialRatio = -1;
    int schedUtilRatio = 100;
    std::string schedRatioPath = "/proc/sys/kernel/sched_util_ratio";
    std::string schedFeaturesPath{};
    bool CheckEnv();
    bool InitSchedParam();
    template<typename T>
    bool SetSchedFeatures(const std::string &filePath, const T &value);
};
}

#endif // !DYNAMICSMT_TUNE_H

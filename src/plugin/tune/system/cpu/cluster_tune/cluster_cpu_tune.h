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
#ifndef CLUSTER_CPU_TUNE_H
#define CLUSTER_CPU_TUNE_H

#include "oeaware/interface.h"

namespace oeaware
{
    class ClusterCpuTune : public Interface
    {
    public:
        ClusterCpuTune();
        ~ClusterCpuTune() override = default;
        Result OpenTopic(const oeaware::Topic &topic) override;
        void CloseTopic(const oeaware::Topic &topic) override;
        void UpdateData(const DataList &dataList) override;
        Result Enable(const std::string &param) override;
        void Disable() override;
        void Run() override;

    private:
		std::string ClusterStatus;
        bool Init();
        bool ReadFile(const std::string &filePath, std::string &value);
        bool ReadClusterFile(std::string &value);
        bool WriteFile(const std::string &filePath, const std::string &value);
        bool WriteClusterFile(const std::string &value);
    };
}

#endif // !CLUSTER_CPU_TUNE_H

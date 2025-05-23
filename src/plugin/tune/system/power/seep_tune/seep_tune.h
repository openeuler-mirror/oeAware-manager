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
#ifndef SEEP_TUNE_H
#define SEEP_TUNE_H

#include "oeaware/interface.h"

namespace oeaware
{
    class Seep : public Interface
    {
    public:
        Seep();
        ~Seep() override = default;
        Result OpenTopic(const oeaware::Topic &topic) override;
        void CloseTopic(const oeaware::Topic &topic) override;
        void UpdateData(const DataList &dataList) override;
        Result Enable(const std::string &param) override;
        void Disable() override;
        void Run() override;

    private:
        int cpuNum = 0;
        std::vector<std::string> cpuScaling;
        bool WriteFile(const std::string &filePath, const std::string &value);
        bool Init();
        bool InsertSeepKo();
        bool WriteSeepFile(const unsigned int &cpu, const std::string &value);
    };
}

#endif // !SEEP_TUNE_H

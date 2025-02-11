/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef CLIENT_ANALYSIS_REPORT_H
#define CLIENT_ANALYSIS_REPORT_H
#include "table.h"

namespace oeaware {

struct AnalysisTemplate {
    std::vector<Table> datas;
    std::vector<Table> conclusions;
    Table suggestions;
};

class AnalysisReport {
public:
    void Init();
    void Print();
    void SetAnalysisTemplate(const AnalysisTemplate &data);
private:
    void PrintTitle(const std::string &title);
    void PrintSubtitle(const std::string &subtitle);
    AnalysisTemplate analysisTemplate;
    const int reportWidth = 120;
};
}

#endif

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
#include "analysis_report.h"
#include <iostream>

namespace oeaware {
void AnalysisReport::Init()
{
}

void AnalysisReport::SetAnalysisTemplate(const AnalysisTemplate &data)
{
    analysisTemplate = data;
}

void AnalysisReport::Print()
{
    PrintTitle("Analysis Report");
    PrintSubtitle("Data Analysis");
    for (size_t i = 1; i <= analysisTemplate.datas.size(); ++i) {
        std::cout << i << ". " << analysisTemplate.datas[i - 1].GetTableName() << std::endl;
        analysisTemplate.datas[i - 1].PrintTable();
    }
    PrintSubtitle("Analysis Conclusion");
    for (size_t i = 0; i < analysisTemplate.conclusions.size(); ++i) {
        std::cout << (i + 1) << ". ";
        analysisTemplate.conclusions[i].PrintTable();
    }
    PrintSubtitle("Analysis Suggestion");
    analysisTemplate.suggestions.PrintTable();
}

void AnalysisReport::PrintTitle(const std::string &title)
{
    std::cout << std::string(reportWidth, '=') << std::endl;
    int cnt = (reportWidth - title.size()) / 2;
    std::cout << std::string(cnt, ' ') << title << std::string(reportWidth - cnt - title.length(), ' ') << std::endl;
    std::cout << std::string(reportWidth, '=') << std::endl;
}

void AnalysisReport::PrintSubtitle(const std::string &subtitle)
{
     int cnt = (reportWidth - subtitle.size()) / 2;
    std::cout << std::string(cnt, '=') << subtitle << std::string(reportWidth - cnt - subtitle.length(), '=')
        << std::endl;
}
}
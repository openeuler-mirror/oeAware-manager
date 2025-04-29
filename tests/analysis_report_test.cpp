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
#include <gtest/gtest.h>
#include "analysis_report.h"
#include "table.h"

TEST(AnalysisReport, Report)
{
    auto &analysisReport = oeaware::AnalysisReport::GetInstance();

    oeaware::AnalysisTemplate analysisTemplate;
    oeaware::Table memoryTable(3, "Memory");
    memoryTable.SetColumnWidth(20);
    memoryTable.AddRow({"metric", "value", "note"});
    memoryTable.AddRow({"l1dtlb_miss", "10%", "high"});
    memoryTable.AddRow({"l1itlb_miss", "1%", "low"});
    memoryTable.AddRow({"l2dtlb_miss", "2%", "low"});
    analysisTemplate.datas["Memory"] = memoryTable;

    oeaware::Table cpuTable(3, "Cpu");
    cpuTable.SetColumnWidth(20);
    cpuTable.AddRow({"metric", "value", "note"});
    cpuTable.AddRow({"CPU usage", "70%", "high"});
    analysisTemplate.datas["CPU"] = cpuTable;

    oeaware::Table c1(1, "c");
    c1.SetBorder(false);
    c1.SetColumnWidth(80);
    c1.AddRow({"tlb miss 过高"});
    oeaware::Table c2(1, "c");
    c2.SetBorder(false);
    c2.SetColumnWidth(80);
    c2.AddRow({"cpu占用率过高"});

    analysisTemplate.conclusions.emplace_back(c1);
    analysisTemplate.conclusions.emplace_back(c2);

    oeaware::Table s(3, "s");
    s.SetColumnWidth(20);
    s.AddRow({"suggestion", "operation", "result"});
    s.AddRow({"use huge page", "oeawarectl -e xxx", "reduce the number of tlb items and reduce the missing rate"});
    analysisTemplate.suggestions = s;
    analysisReport.SetAnalysisTemplate(analysisTemplate);
    analysisReport.Print();
}
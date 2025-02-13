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
#include <securec.h>
#include "oe_client.h"
#include "data_register.h"

namespace oeaware {
static int CallBack(const DataList *dataList)
{
    std::string topicName(dataList->topic.topicName);
    std::string instanceName(dataList->topic.instanceName);
    auto &analysisReport = AnalysisReport::GetInstance();
    if (topicName == MEMORY_ANALYSIS) {
        TlbMiss tempTlbMiss;
        auto *memoryAnalysisData = static_cast<MemoryAnalysisData*>(dataList->data[0]);
        analysisReport.UpdateMemoryData(*memoryAnalysisData);
    }
	return 0;
}

void AnalysisReport::UpdateMemoryData(const MemoryAnalysisData &memoryAnalysisData)
{
    UpdateTlbMiss(memoryAnalysisData.tlbMiss);
}

void AnalysisReport::UpdateTlbMiss(const TlbMiss &tempTlbMiss)
{
    tlbMissAnalysis.Add(tempTlbMiss);
}

void AnalysisReport::Init(const std::vector<std::string> &topics)
{
    for (auto &topic : topics) {
        char *name = new char[topic.size() + 1];
        strcpy_s(name, topic.size() + 1, topic.data());
        CTopic cTopic{OE_ANALYSIS_AWARE, name, ""};
        OeSubscribe(&cTopic, CallBack);
        delete []name;
    }
    analysisTemplate.suggestions.Init(DEFAULT_ROW, "suggestion");
    analysisTemplate.suggestions.SetColumnWidth(DEFAULT_SUGGESTION_WIDTH);
    analysisTemplate.suggestions.AddRow({"suggestion", "operation", "result"});
}

void AnalysisReport::SetAnalysisTemplate(const AnalysisTemplate &data)
{
    analysisTemplate = data;
}

void AnalysisReport::MemoryAnalyze()
{
    oeaware::Table memoryTable(DEFAULT_ROW, "Memory");
    memoryTable.SetColumnWidth(DEFAULT_SUGGESTION_WIDTH);
    const TlbMiss &tlbMiss = tlbMissAnalysis.tlbMiss;
    int cnt = tlbMissAnalysis.cnt;
    double l1dTlbMiss = tlbMiss.l1dTlbMiss * PERCENT / cnt;
    double l1iTlbMiss = tlbMiss.l1iTlbMiss * PERCENT / cnt;
    double l2dTlbMiss = tlbMiss.l2dTlbMiss * PERCENT / cnt;
    double l2iTlbMiss = tlbMiss.l2iTlbMiss * PERCENT / cnt;
    memoryTable.AddRow({"metric", "value", "note"});
    memoryTable.AddRow({"l1dtlb_miss", std::to_string(l1dTlbMiss) + "%", (l1dTlbMiss > tlbMissAnalysis.threshold1 ?
        "high" : "low")});
    memoryTable.AddRow({"l1itlb_miss", std::to_string(l1iTlbMiss) + "%", (l1iTlbMiss > tlbMissAnalysis.threshold1 ?
        "high" : "low")});
    memoryTable.AddRow({"l2dtlb_miss", std::to_string(l2dTlbMiss) + "%", (l2dTlbMiss > tlbMissAnalysis.threshold2 ?
        "high" : "low")});
    memoryTable.AddRow({"l2itlb_miss", std::to_string(l2iTlbMiss) + "%", (l2iTlbMiss > tlbMissAnalysis.threshold2 ?
        "high" : "low")});
    analysisTemplate.datas.emplace_back(memoryTable);
    if (tlbMissAnalysis.IsHighMiss()) {
        oeaware::Table highTlbMiss(1, "higTlbMiss");
        highTlbMiss.SetBorder(false);
        highTlbMiss.SetColumnWidth(DEFAULT_CONCLUSION_WIDTH);
        highTlbMiss.AddRow({"The tlbmiss is too high."});
        analysisTemplate.conclusions.emplace_back(highTlbMiss);
        analysisTemplate.suggestions.AddRow({"use huge page", "oeawarectl -e transparent_hugepage_tune",
            "reduce the number of tlb items and reduce the missing rate"});
    }
}

void AnalysisReport::AnalyzeResult()
{
    MemoryAnalyze();
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
    std::cout << std::string(cnt, '=') << subtitle << std::string(reportWidth - cnt - subtitle.length(), '=') <<
        std::endl;
}

void TlbMissAnalysis::Add(const TlbMiss &tempTlbMiss)
{
    tlbMiss.l1iTlbMiss += tempTlbMiss.l1iTlbMiss;
    tlbMiss.l1dTlbMiss += tempTlbMiss.l1dTlbMiss;
    tlbMiss.l2iTlbMiss += tempTlbMiss.l2iTlbMiss;
    tlbMiss.l2dTlbMiss += tempTlbMiss.l2dTlbMiss;
    cnt++;
}

bool TlbMissAnalysis::IsHighMiss()
{
    return tlbMiss.l1dTlbMiss * PERCENT / cnt >= threshold1 ||
        tlbMiss.l1iTlbMiss * PERCENT / cnt >= threshold1 ||
        tlbMiss.l2dTlbMiss * PERCENT / cnt >= threshold2 ||
        tlbMiss.l2iTlbMiss * PERCENT / cnt >= threshold2;
}
}
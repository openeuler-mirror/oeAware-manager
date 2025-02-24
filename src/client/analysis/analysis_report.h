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
#include "oeaware/data/analysis_data.h"

namespace oeaware {
const int DEFAULT_ROW = 3;
const int PERCENT = 100;
const int DEFAULT_SUGGESTION_WIDTH = 20;
const int DEFAULT_CONCLUSION_WIDTH = 80;
struct AnalysisTemplate {
    std::vector<Table> datas;
    std::vector<Table> conclusions;
    Table suggestions;
};

struct TlbMissAnalysis {
    TlbMiss tlbMiss;
    int cnt = -1; // The first data is invalid.
    int threshold1 = 5;
    int threshold2 = 5;
    bool IsHighMiss();
    void Add(const TlbMiss &tempTlbMiss);
};

class AnalysisReport {
public:
    static AnalysisReport &GetInstance()
    {
        static AnalysisReport instance;
        return instance;
    }
    AnalysisReport(const AnalysisReport &) = delete;
    AnalysisReport &operator=(const AnalysisReport &) = delete;
    void Init(const std::vector<std::string> &topics);
    void Print();
    void SetAnalysisTemplate(const AnalysisTemplate &data);
    void UpdateMemoryData(const MemoryAnalysisData &memoryAnalysisData);
    void UpdateTlbMiss(const TlbMiss &tempTlbMiss);
    void AnalyzeResult();
private:
    AnalysisReport() {}
    ~AnalysisReport() {}
    void MemoryAnalyze();
    void PrintTitle(const std::string &title);
    void PrintSubtitle(const std::string &subtitle);
    
    AnalysisTemplate analysisTemplate;
    TlbMissAnalysis tlbMissAnalysis;
    const int reportWidth = 120;
};
}

#endif

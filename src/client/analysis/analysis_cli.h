/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef __ANALYSIS_CLI_H__
#define __ANALYSIS_CLI_H__

#include <string>
#include <unordered_map>
#include "config.h"
#include "oeaware/data/analysis_data.h"
#include "oeaware/data_list.h"
class AnalysisCli {
public:
    static AnalysisCli &GetInstance()
    {
        static AnalysisCli instance;
        return instance;
    }
    AnalysisCli(const AnalysisCli &) = delete;
    AnalysisCli &operator=(const AnalysisCli &) = delete;
    struct ReportStat {
        bool isFinish = false;
        AnalysisReport report;
    };
    void StopRun();
    void Update(AnalysisReport *report);
    int Proc(int argc, char **argv);
    bool IsAnalysisMode(int argc, char **argv);
private:
    AnalysisCli() {}
    ~AnalysisCli() {}
    int analysisTime = 0;
    int timeout = 3;
    bool isShowRealTimeReport = false;
    bool runRealTimeAnalysis = true;
    bool isShowVerbose = false;
    std::string realtimeReport;
    std::string summaryReport;
    Config config;
    bool rtRptFinish = false;
    bool summaryRptFinish = false;
    bool Init();
    void Run();
    void PrintReport(bool isSummary);
    void Exit();
};

#endif
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
#include "analysis_cli.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <csignal>
#include <iomanip>
// sdk
#include "oe_client.h"
// analysis output
#include "oeaware/data/analysis_data.h"
// own
#include "config.h"
#include "analysis_report.h"

static int CallBack(const DataList *dataList)
{
	if (dataList && dataList->len && dataList->data) {
        AnalysisReport *data = static_cast<AnalysisReport *>(dataList->data[0]);
        AnalysisCli::GetInstance().Update(data);
    }
	return 0;
}
/*
 * @progress : precentage of the progress, range from 0 to 1
 * @barWidth : width of the progress bar
 */
static void PrintProgressBar(const std::string &head, float progress, int barWidth = 50)
{
    if (progress < 0) {
        progress = 0;
    } else if (progress > 1) {
        progress = 1;
    }
    // calculate the position of the progress bar
    int pos = static_cast<int>(barWidth * progress);

    std::cout << head << " [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) {
            std::cout << "=";
        } else if (i == pos) {
            std::cout << ">";
        } else {
            std::cout << " ";
        }
    }
    // 3 is used for width, 100.0f is used for percentage conversion
    std::cout << "] " << std::setw(3) << static_cast<int>(progress * 100.0f) << "%\r";
    std::cout.flush();
}

void AnalysisCli::Run()
{
    auto &analysisReport = oeaware::AnalysisReport::GetInstance();
    analysisReport.Init(std::vector<std::string>{MEMORY_ANALYSIS}, config);
    sleep(analysisTime);
    analysisReport.AnalyzeResult();
    analysisReport.Print();
}

void AnalysisCli::StopRun()
{
    rtRptFinish = true;
    runRealTimeAnalysis = false;
}

bool AnalysisCli::Init()
{
    analysisTime = config.GetAnalysisTimeMs();
    isShowRealTimeReport = config.IsShowRealTimeReport();
    isShowVerbose = config.ShowVerbose();
    int ret = OeInit();
    if (ret != 0) {
        std::cout << " SDK(Analysis) Init failed , result " << ret << std::endl;
    }
    if (isShowVerbose) {
        std::cout << "SDK(Analysis) Init result " << ret << std::endl;
    }
    return ret == 0;
}

void AnalysisCli::Update(AnalysisReport *report)
{
    if (report == nullptr || report->data == nullptr) {
        return;
    }
    if (report->reportType == ANALYSIS_REPORT_REALTIME) {
        realtimeReport = std::string(report->data);
        rtRptFinish = true;
    } else {
        summaryReport = std::string(report->data);
        summaryRptFinish = true;
    }
}

void AnalysisCli::PrintReport(bool isSummary)
{
    if (isSummary) {
        std::cout << summaryReport << std::endl;
    } else if (isShowRealTimeReport) {
        std::cout << realtimeReport << std::endl;
    }
}

void AnalysisCli::Exit()
{
    runRealTimeAnalysis = false;
    OeSubscribe(&topicSummaryRpt, CallBack);
    while (!summaryRptFinish) {
        usleep(10); // 10us sleep to avoid busy loop
    }
    PrintReport(true);
    OeUnsubscribe(&topicSummaryRpt);
    OeUnsubscribe(&topicRtRpt);
}

bool AnalysisCli::IsAnalysisMode(int argc, char **argv)
{
    // analysis mode format : oeawarectl analysis xxxx
    const std::string analysisMode = "analysis";
    // argc >= 2 means there is at least one parameter
    if (argc >= 2 && std::string(argv[1]) == analysisMode) {
        return true;
    }
    return false;
}

void AnalysisQuitHandler(int signum)
{
    (void)signum;
    AnalysisCli::GetInstance().StopRun();
}

int AnalysisCli::Proc(int argc, char **argv)
{
    // skip oeawarectl
    argc -= 1;
    argv += 1;
    if (!config.Init(argc, argv)) {
        return -1;
    }
    AnalysisCli &cli = AnalysisCli::GetInstance();
    if (!cli.Init()) {
        return -1;
    }
    cli.Run();
    return 0;
}

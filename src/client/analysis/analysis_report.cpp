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
#include <iostream>
#include <fstream>
#include <securec.h>
#include "oeaware/default_path.h"
#include "oe_client.h"
#include "config.h"
#include "data_register.h"
#include "analysis_report.h"

namespace oeaware {
static int CallBack(const DataList *dataList)
{
    std::string topicName(dataList->topic.topicName);
    std::string instanceName(dataList->topic.instanceName);
    auto &analysisReport = AnalysisReport::GetInstance();
    auto recv = static_cast<AnalysisResultItem*>(dataList->data[0]);
    analysisReport.AddAnalysisReportItem(recv);
    return 0;
}

void AnalysisReport::AddAnalysisReportItem(AnalysisResultItem *analysisResultItem)
{
    for (int i = 0; i < analysisResultItem->dataItemLen; ++i) {
        std::string metric = analysisResultItem->dataItem[i].metric;
        std::string value = analysisResultItem->dataItem[i].value;
        std::string extra = analysisResultItem->dataItem[i].extra;
        if (analysisResultItem->dataItem[i].type == DATA_TYPE_CPU) {
            analysisTemplate.datas["CPU"].AddRow({metric, value, extra});
        } else if (analysisResultItem->dataItem[i].type == DATA_TYPE_MEMORY) {
            analysisTemplate.datas["Memory"].AddRow({metric, value, extra});
        } else if (analysisResultItem->dataItem[i].type == DATA_TYPE_IO) {
            analysisTemplate.datas["IO"].AddRow({metric, value, extra});
        } else if (analysisResultItem->dataItem[i].type == DATA_TYPE_NETWORK) {
            analysisTemplate.datas["Network"].AddRow({metric, value, extra});
#ifdef __riscv
        } else if (analysisResultItem->dataItem[i].type == DATA_TYPE_HWPROBE) {
            analysisTemplate.datas["Hwprobe"].AddRow({metric, value, extra});
        }
#else
        }
#endif
    }
    Table conclusion(1, "conclusion");
    conclusion.SetBorder(false);
    conclusion.SetColumnWidth(DEFAULT_CONCLUSION_WIDTH);
    conclusion.AddRow({std::string(analysisResultItem->conclusion)});
    analysisTemplate.conclusions.emplace_back(conclusion);
    analysisTemplate.suggestions.AddRow({std::string(analysisResultItem->suggestionItem.suggestion),
        std::string(analysisResultItem->suggestionItem.opt), std::string(analysisResultItem->suggestionItem.extra)});
}

void AnalysisReport::AddAnalysisTopic(const std::string &insName, const std::string &topicName,
    const std::vector<std::string> &params)
{
    std::string param = "";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i) param += ",";
        param += params[i];
    }
    topics.emplace_back(std::vector<std::string>{insName, topicName, param});
}

void AnalysisReport::Init(Config &config)
{
    std::string configPath = DEFAULT_ANALYSYS_PATH;
    if (!config.LoadConfig(configPath)) {
        std::cerr << "Warning: Failed to load configuration from " << configPath
                  << ", using default values." << std::endl;
    }
#ifdef __riscv
    std::string timeParam = "t:" + std::to_string(config.GetAnalysisTimeMs());
    
    std::string threshold1 = "threshold1:" + std::to_string(config.GetL1MissThreshold());
    std::string threshold2 = "threshold2:" + std::to_string(config.GetL2MissThreshold());
    AddAnalysisTopic("hugepage_analysis", "hugepage", {timeParam, threshold1, threshold2});

    std::string smcthreshold1 = "threshold1:" + std::to_string(config.GetSmcChangeRate());
    std::string smcthreshold2 = "threshold2:" + std::to_string(config.GetSmcLoNetFlow());
    AddAnalysisTopic("smc_d_analysis", "smc_d", {timeParam, smcthreshold1, smcthreshold2});

    std::string threshold = "threshold:" + std::to_string(config.GetDynamicSmtThreshold());
    AddAnalysisTopic("dynamic_smt_analysis", "dynamic_smt", {timeParam, threshold});

    std::string hostCpuUsage = "hostCpuUsageThreshold:" +
                std::to_string(config.GetDockerCoordinationBurstHostCpuUsageThreshold());
    std::string dockerCpuUsage = "dockerCpuUsageThreshold:" +
                std::to_string(config.GetDockerCoordinationBurstDockerCpuUsageThreshold());
    AddAnalysisTopic("docker_coordination_burst_analysis", "docker_coordination_burst",
                     {timeParam, hostCpuUsage, dockerCpuUsage});

    std::string threadThreshold = "threshold:" + std::to_string(config.GetNumaThreadThreshold());
    AddAnalysisTopic("numa_analysis", "numa_analysis", {timeParam, threadThreshold});

    AddAnalysisTopic(OE_NET_HIRQ_ANALYSIS, OE_NET_HIRQ_ANALYSIS, {timeParam});

    AddAnalysisTopic(OE_HWPROBE_ANALYSIS, OE_HWPROBE_ANALYSIS, {timeParam});
#else
    std::string timeParam = "t:" + std::to_string(config.GetAnalysisTimeMs());
    std::string threshold1 = "threshold1:" + std::to_string(config.GetL1MissThreshold());
    std::string threshold2 = "threshold2:" + std::to_string(config.GetL2MissThreshold());
    AddAnalysisTopic("hugepage_analysis", "hugepage", {timeParam, threshold1, threshold2});

    std::string threshold = "threshold:" + std::to_string(config.GetDynamicSmtThreshold());
    AddAnalysisTopic("dynamic_smt_analysis", "dynamic_smt", {timeParam, threshold});

    std::string smcthreshold1 = "threshold1:" + std::to_string(config.GetSmcChangeRate());
    std::string smcthreshold2 = "threshold2:" + std::to_string(config.GetSmcLoNetFlow());
    AddAnalysisTopic("smc_d_analysis", "smc_d", {timeParam, smcthreshold1, smcthreshold2});

    std::string pid = "pid:" + std::to_string(config.GetPid());
    std::string xcallThreshold = "threshold:" + std::to_string(config.GetXcallThreshold());
    std::string xcallTopNum = "num:" + std::to_string(config.GetXcallTopNum());
    AddAnalysisTopic(OE_XCALL_ANALYSIS, "xcall", {timeParam, pid, xcallThreshold, xcallTopNum});

    AddAnalysisTopic(OE_NET_HIRQ_ANALYSIS, OE_NET_HIRQ_ANALYSIS, {timeParam});

    std::string threadThreshold = "threshold:" + std::to_string(config.GetNumaThreadThreshold());
    AddAnalysisTopic("numa_analysis", "numa_analysis", {timeParam, threadThreshold});
    std::string hostCpuUsage = "hostCpuUsageThreshold:" +
                std::to_string(config.GetDockerCoordinationBurstHostCpuUsageThreshold());
    std::string dockerCpuUsage = "dockerCpuUsageThreshold:" +
                std::to_string(config.GetDockerCoordinationBurstDockerCpuUsageThreshold());
    AddAnalysisTopic("docker_coordination_burst_analysis", "docker_coordination_burst",
                     {timeParam, hostCpuUsage, dockerCpuUsage});

    std::string microarchTidNoCmpConfigStream = "micro_arch_tidnocmp_config_stream:" +
                                                 config.GetMicroArchTidNoCmpConfigStream();
    AddAnalysisTopic("microarch_tidnocmp_analysis", "microarch_tidnocmp",
                     {timeParam, microarchTidNoCmpConfigStream});
#endif
    const int INS_NAME_INDEX = 0;
    const int TOPIC_NAME_INDEX = 1;
    const int PARAM_INDEX = 2;
    for (auto &topic : topics) {
        char *insName = new char[topic[INS_NAME_INDEX].size() + 1];
        errno_t ret = strcpy_s(insName, topic[INS_NAME_INDEX].size() + 1, topic[INS_NAME_INDEX].data());
        if (ret != EOK) {
            std::cerr << "Error: insName strcpy_s failed with error code " << ret << std::endl;
            return;
        }
        char *topicName = new char[topic[TOPIC_NAME_INDEX].size() + 1];
        ret = strcpy_s(topicName, topic[TOPIC_NAME_INDEX].size() + 1, topic[TOPIC_NAME_INDEX].data());
        if (ret != EOK) {
            std::cerr << "Error: topicName strcpy_s failed with error code " << ret << std::endl;
            return;
        }
        char *param = new char[topic[PARAM_INDEX].size() + 1];
        ret = strcpy_s(param, topic[PARAM_INDEX].size() + 1, topic[PARAM_INDEX].data());
        if (ret != EOK) {
            std::cerr << "Error: param strcpy_s failed with error code " << ret << std::endl;
            return;
        }
        CTopic cTopic{ insName, topicName, param };
        OeSubscribe(&cTopic, CallBack);
        delete []insName;
        delete []topicName;
        delete []param;
    }
    analysisTemplate.suggestions.Init(DEFAULT_ROW, "suggestion");
    analysisTemplate.suggestions.SetColumnWidth(DEFAULT_SUGGESTION_WIDTH);
    analysisTemplate.suggestions.AddRow({"suggestion", "operation", "result"});

    oeaware::Table memoryTable(DEFAULT_ROW, "Memory");
    memoryTable.SetColumnWidth(DEFAULT_SUGGESTION_WIDTH);
    memoryTable.AddRow({"metric", "value", "note"});
    analysisTemplate.datas["Memory"] = memoryTable;

    oeaware::Table cpuTable(DEFAULT_ROW, "CPU");
    cpuTable.SetColumnWidth(DEFAULT_SUGGESTION_WIDTH);
    cpuTable.AddRow({"metric", "value", "note"});
    analysisTemplate.datas["CPU"] = cpuTable;
    oeaware::Table ioTable(DEFAULT_ROW, "IO");
    ioTable.SetColumnWidth(DEFAULT_SUGGESTION_WIDTH);
    ioTable.AddRow({"metric", "value", "note"});
    analysisTemplate.datas["IO"] = ioTable;
    oeaware::Table networkTable(DEFAULT_ROW, "Network");
    networkTable.SetColumnWidth(DEFAULT_SUGGESTION_WIDTH);
    networkTable.AddRow({"metric", "value", "note"});
    analysisTemplate.datas["Network"] = networkTable;

#ifdef __riscv
    oeaware::Table hwprobeTable(DEFAULT_ROW, "Hwprobe");
    hwprobeTable.SetColumnWidth(DEFAULT_SUGGESTION_WIDTH);
    hwprobeTable.AddRow({"metric", "value", "note"});
    analysisTemplate.datas["Hwprobe"] = hwprobeTable;
#endif
}

void AnalysisReport::SetAnalysisTemplate(const AnalysisTemplate &data)
{
    analysisTemplate = data;
}

void AnalysisReport::Print()
{
    PrintTitle("Analysis Report");
    PrintSubtitle("Data Analysis");
    int index = 1;
    for (auto &p : analysisTemplate.datas) {
        if (p.second.GetRowCount() > 1) {
            std::cout << index++ << ". " << p.second.GetTableName() << std::endl;
            p.second.PrintTable();
        }
    }
    PrintSubtitle("Analysis Conclusion");
    index = 1;
    for (size_t i = 0; i < analysisTemplate.conclusions.size(); ++i) {
        auto conclusion = analysisTemplate.conclusions[i];
        auto content = conclusion.GetContent(0, 0);
        if (!content.empty()) {
            std::cout << index++ << ". ";
            conclusion.PrintTable();
        }
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

void AnalysisReport::PrintMarkDown(const std::string &path)
{
    std::ofstream out(path + "/analysis_report.md");
    if (out.is_open()) {
        out << "## Data Analysis\n";
        int index = 1;
        for (auto data : analysisTemplate.datas) {
            auto name = data.first;
            out << index++ << ". " << name << "\n\n";
            out << data.second.GetMarkDownTable();
        }
        out << "## Conclusion\n";
        index = 1;
        for (auto conclusion : analysisTemplate.conclusions) {
            auto content = conclusion.GetContent(0, 0);
            if (!content.empty()) {
                out << index++ << ". " << content << "\n";
            }
        }
        out << "## Suggestion\n";
        out << analysisTemplate.suggestions.GetMarkDownTable();
        out.close();
    }
}
}
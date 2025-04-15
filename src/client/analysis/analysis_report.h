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
#include <unordered_map>
#include "table.h"
#include "oeaware/data/analysis_data.h"
#include "config.h"

namespace oeaware {
const int DEFAULT_ROW = 3;
const int PERCENT = 100;
const int DEFAULT_SUGGESTION_WIDTH = 20;
const int DEFAULT_CONCLUSION_WIDTH = 80;
struct AnalysisTemplate {
    std::unordered_map<std::string, Table> datas;
    std::vector<Table> conclusions;
    Table suggestions;
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
    void Init(const Config &config);
    void Print();
    void SetAnalysisTemplate(const AnalysisTemplate &data);
    void AddAnalysisReportItem(AnalysisResultItem *analysisResultItem, const std::string &name);
private:
    AnalysisReport() {}
    ~AnalysisReport() {}
    // @brief: Add a analysis topic.
    // @param insName Instance name.
    // @param topicName Topic name.
    // @param params Analysis topic param, for example
    // t:10,c:20      a comma-separated (key:value) pair.
    void AddAnalysisTopic(const std::string &insName, const std::string &topicName,
    const std::vector<std::string> &params);
    void PrintTitle(const std::string &title);
    void PrintSubtitle(const std::string &subtitle);
    std::vector<std::vector<std::string>> topics;
    AnalysisTemplate analysisTemplate;
    const int reportWidth = 120;
};
}

#endif

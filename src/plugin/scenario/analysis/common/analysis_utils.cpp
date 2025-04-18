/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include "analysis_utils.h"

namespace oeaware {
void CreateAnalysisResultItem(const std::vector<std::vector<std::string>> &metrics, const std::string &conclusion,
    const std::vector<std::string> &suggestionItem, AnalysisResultItem *analysisResultItem)
{
    auto len = metrics.size();
    if (len == 0) {
        return;
    }
    analysisResultItem->dataItemLen = len;
    analysisResultItem->dataItem = new DataItem[len];
    const int METRIC_INDEX = 0;
    const int VALUE_INDEX = 1;
    const int EXTRA_INDEX = 2;
    for (size_t i = 0; i < len; ++i) {
        auto metric = metrics[i][METRIC_INDEX];
        auto value = metrics[i][VALUE_INDEX];
        auto extra = metrics[i][EXTRA_INDEX];
        analysisResultItem->dataItem[i].metric = new char[metric.size() + 1];
        strcpy_s(analysisResultItem->dataItem[i].metric, metric.size() + 1, metric.data());
        analysisResultItem->dataItem[i].value = new char[value.size() + 1];
        strcpy_s(analysisResultItem->dataItem[i].value, value.size() + 1, value.data());
        analysisResultItem->dataItem[i].extra = new char[extra.size() + 1];
        strcpy_s(analysisResultItem->dataItem[i].extra, extra.size() + 1, extra.data());
    }
    analysisResultItem->conclusion = new char[conclusion.size() + 1];
    strcpy_s(analysisResultItem->conclusion, conclusion.size() + 1, conclusion.data());
    std::string suggestion = "";
    std::string opt = "";
    std::string extra = "";
    if (suggestionItem.size() > 1) {
        const int SUGGESTION_INDEX = 0;
        const int OPT_INDEX = 1;
        suggestion = suggestionItem[SUGGESTION_INDEX];
        opt = suggestionItem[OPT_INDEX];
        extra = suggestionItem[EXTRA_INDEX];
    }
    analysisResultItem->suggestionItem.suggestion = new char[suggestion.size() + 1];
    strcpy_s(analysisResultItem->suggestionItem.suggestion, suggestion.size() + 1, suggestion.data());
    analysisResultItem->suggestionItem.opt = new char[opt.size() + 1];
    strcpy_s(analysisResultItem->suggestionItem.opt, opt.size() + 1, opt.data());
    analysisResultItem->suggestionItem.extra = new char[extra.size() + 1];
    strcpy_s(analysisResultItem->suggestionItem.extra, extra.size() + 1, extra.data());
}
}
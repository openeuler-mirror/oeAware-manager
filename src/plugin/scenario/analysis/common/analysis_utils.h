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
#ifndef ANALYSIS_UTILS_H
#define ANALYSIS_UTILS_H
#include <vector>
#include <string>
#include <securec.h>
#include "oeaware/data/analysis_data.h"
namespace oeaware {
const int PERCENTAGE_FACTOR = 100;
const int DYNAMIC_SMT_THRESHOLD = 40;
const int THP_THRESHOLD1 = 5;
const int THP_THRESHOLD2 = 10;
const int ANALYSIS_TIME_PERIOD = 1000;
void CreateAnalysisResultItem(const std::vector<std::vector<std::string>> &metrics, const std::string &conclusion,
    const std::vector<std::string> &suggestionItem, const std::vector<int> type, AnalysisResultItem *analysisResultItem);
}
#endif

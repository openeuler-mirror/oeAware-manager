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

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <getopt.h>
#include <iostream>

const int L1_MISS_THRESHOLD = 200;
const int L2_MISS_THRESHOLD = 201;
const int OUT_PATH = 202;
class Config {
public:
    bool Init(int argc, char **argv);
    int GetAnalysisTimeMs() const
    {
        return analysisTime;
    }
    bool IsShowRealTimeReport() const
    {
        return isShowRealTimeReport;
    }
    bool ShowVerbose() const
    {
        return showVerbose;
    }
    double GetL1MissThreshold() const
    {
        return l1MissThreshold;
    }
    double GetL2MissThreshold() const
    {
        return l2MissThreshold;
    }
    std::string GetOutMarkDownPath() const
    {
        return outMarkDownPath;
    }
private:
    const int minAnalyzeTime = 1;
    const int maxAnalyzeTime = 100;
    int analysisTime = 30; // default 30s
    double l1MissThreshold = 5;
    double l2MissThreshold = 10;
    std::string  outMarkDownPath = "";
    const std::string shortOptions = "t:hrv";
    const std::vector<option> longOptions = {
        {"help", no_argument, nullptr, 'h'},
        {"realtime", no_argument, nullptr, 'r'},
        {"time", required_argument, nullptr, 't'},
        {"verbose", no_argument, nullptr, 'v'},
        {"l1-miss-threshold", required_argument, nullptr, L1_MISS_THRESHOLD},
        {"l2-miss-threshold", required_argument, nullptr, L2_MISS_THRESHOLD},
        {"out-path", required_argument, nullptr, OUT_PATH},
        {nullptr, 0, nullptr, 0}
    };
    bool isShowRealTimeReport = false;
    bool showVerbose = false;
    bool ParseTime(const char *arg);
    void PrintHelp();
};


#endif
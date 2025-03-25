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

#include "config.h"
#include <regex>

void Config::PrintHelp()
{
    std::string usage = "";
    usage += "usage: oeawarectl analysis [options]...\n";
    usage += "  options\n";
    usage += "   -t|--time <s>              set analysis duration in seconds(default "\
        + std::to_string(analysisTime) + "s), range from " + std::to_string(minAnalyzeTime) \
        + " to " + std::to_string(maxAnalyzeTime) + ".\n";
    usage += "   -r|--realtime              show real time report.\n";
    usage += "   -v|--verbose               show verbose information.\n";
    usage += "   -h|--help                  show this help message.\n";
    usage += "   --l1-miss-threshold                  set l1 tlbmiss threshold.\n";
    usage += "   --l2-miss-threshold                  set l2 tlbmiss threshold.\n";
    std::cout << usage;
}

bool Config::ParseTime(const char *arg)
{
    char *endptr;
    long time = std::strtol(arg, &endptr, 10);
    if (arg == endptr || *endptr != '\0' || time < minAnalyzeTime || time > maxAnalyzeTime) {
        std::cerr << "Error: Invalid time value '" << arg << "' (must be between "
            + std::to_string(minAnalyzeTime) + " and " + std::to_string(maxAnalyzeTime) + ").\n";
        return false;
    }
    analysisTime = static_cast<int>(time);
    return true;
}

static bool IsNum(const std::string &s)
{
    std::regex num(R"(^[+]?\d+(\.\d+)?$)");
    return std::regex_match(s, num);
}

bool Config::Init(int argc, char **argv)
{
    if (argv == nullptr) {
        return false;
    }
    int opt;
    while ((opt = getopt_long(argc, argv, shortOptions.c_str(), longOptions.data(), nullptr)) != -1) {
        switch (opt) {
            case 't':
                if (!ParseTime(optarg)) {
                    PrintHelp();
                    return false;
                }
                break;
            case 'r':
                isShowRealTimeReport = true;
                break;
            case 'v':
                showVerbose = true;
                break;
            case L1_MISS_THRESHOLD:
                if (!IsNum(optarg)) {
                    std::cerr << "Error: Invalid l1-miss-threshold: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                l1MissThreshold = atof(optarg);
                break;
            case L2_MISS_THRESHOLD:
                if (!IsNum(optarg)) {
                    std::cerr << "Error: Invalid l2-miss-threshold: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                l2MissThreshold = atof(optarg);
                break;
            case 'h':
            default:
                PrintHelp();
                return false;
        }
    }
    if (optind != argc) {
        PrintHelp();
        return false;
    }
    return true;
}

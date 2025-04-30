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

#include <regex>
#include <oeaware/utils.h>
#include <fstream>
#include "config.h"

void Config::PrintHelp()
{
    std::string usage = "";
    usage += "usage: oeawarectl analysis [options]...\n";
    usage += "  options\n";
    usage += "   -t|--time <s>              set analysis duration in seconds(default " +
             std::to_string(analysisTime) + "s), range from " + std::to_string(minAnalyzeTime) +
             " to " + std::to_string(maxAnalyzeTime) + ".\n";
    usage += "   -r|--realtime              show real time report.\n";
    usage += "   -v|--verbose               show verbose information.\n";
    usage += "   -h|--help                  show this help message.\n";
    usage += "   --l1-miss-threshold                  set l1 tlbmiss threshold.\n";
    usage += "   --l2-miss-threshold                  set l2 tlbmiss threshold.\n";
    usage += "   --out-path                           set the path of the analysis report.\n";
    usage += "   --dynamic-smt-threshold              set dynamic smt cpu threshold.\n";
    usage += "   --pid                      set the pid to be analyzed.\n";
    usage += "   --numa-thread-threshold              set numa sched thread creation threshold.\n";
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
                if (!oeaware::IsNum(optarg)) {
                    std::cerr << "Error: Invalid l1-miss-threshold: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                l1MissThreshold = atof(optarg);
                l1MissThresholdSet = true;
                break;
            case L2_MISS_THRESHOLD:
                if (!oeaware::IsNum(optarg)) {
                    std::cerr << "Error: Invalid l2-miss-threshold: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                l2MissThreshold = atof(optarg);
                l2MissThresholdSet = true;
                break;
            case OUT_PATH: {
                std::string path(optarg);
                if (oeaware::FileExist(path)) {
                    outMarkDownPath = optarg;
                } else {
                    std::cerr << "Error: Invalid out-path: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                break;
            }
            case DYNAMIC_SMT_THRESHOLD:
                if (!oeaware::IsNum(optarg)) {
                    std::cerr << "Error: Invalid dynamic-smt-threshold: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                dynamicSmtThreshold = atof(optarg);
                dynamicSmtThresholdSet = true;
                break;
            case PID:
                if (!oeaware::IsNum(optarg)) {
                    std::cerr << "Error: Invalid pid: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                pid = atoi(optarg);
            case NUMA_THREAD_THRESHOLD:
                if (!oeaware::IsNum(optarg)) {
                    std::cerr << "Error: Invalid numa thread threshold: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                numaThreadThreshold = atoi(optarg);
                numaThreadThresholdSet = true;
                break;
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

bool Config::LoadConfig(const std::string& configPath)
{
    try {
        YAML::Node config = YAML::LoadFile(configPath);
        if (config["dynamic_smt"]) {
            auto dynamic_smt = config["dynamic_smt"];
            if (dynamic_smt["threshold"] && !dynamicSmtThresholdSet) {
                const_cast<Config*>(this)->dynamicSmtThreshold = dynamic_smt["threshold"].as<double>();
            }
        }

        if (config["hugepage"]) {
            auto hugepage = config["hugepage"];
            if (hugepage["l1_miss_threshold"] && !l1MissThresholdSet) {
                const_cast<Config*>(this)->l1MissThreshold = hugepage["l1_miss_threshold"].as<double>();
            }
            if (hugepage["l2_miss_threshold"] && !l2MissThresholdSet) {
                const_cast<Config*>(this)->l2MissThreshold = hugepage["l2_miss_threshold"].as<double>();
            }
        }

        if (config["numa_analysis"]) {
            auto numa_analysis = config["numa_analysis"];
            if (numa_analysis["thread_threshold"] && !numaThreadThresholdSet) {
                const_cast<Config*>(this)->numaThreadThreshold = numa_analysis["thread_threshold"].as<int>();
            }
        }

        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
}

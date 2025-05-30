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
#include <sstream>
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
    usage += "   --smc-change-rate              set smc connections change rate threshold.\n";
    usage += "   --smc-localnet-flow              set smc local net flow threshold.\n";
    usage += "   --host-cpu-usage-threshold                  set host cpu usage threshold.\n";
    usage += "   --docker-cpu-usage-threshold                  set docker cpu usage threshold.\n";
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
                if (l1MissThreshold < 0 || l1MissThreshold > THRESHOLD_UP) {
                    std::cerr << "Warn: analysis config 'hugepage:l1_miss_threshold(" << optarg <<
                    ")' value must be [0, 100].\n";
                    PrintHelp();
                    return false;
                }
                l1MissThresholdSet = true;
                break;
            case L2_MISS_THRESHOLD:
                if (!oeaware::IsNum(optarg)) {
                    std::cerr << "Error: Invalid l2-miss-threshold: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                l2MissThreshold = atof(optarg);
                if (l2MissThreshold < 0 || l2MissThreshold > THRESHOLD_UP) {
                    std::cerr << "Warn: analysis config 'hugepage:l2_miss_threshold(" << optarg <<
                    ")' value must be [0, 100].\n";
                    PrintHelp();
                    return false;
                }
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
                if (dynamicSmtThreshold < 0 || dynamicSmtThreshold > THRESHOLD_UP) {
                    std::cerr << "Warn: analysis config 'dynamic_smt:threshold(" << optarg <<
                    ")' value must be [0, 100].\n";
                    PrintHelp();
                    return false;
                }
                dynamicSmtThresholdSet = true;
                break;
            case PID:
                if (!oeaware::IsInteger(optarg)) {
                    std::cerr << "Error: Invalid pid: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                pid = atoi(optarg);
                if (pid < 0) {
                    std::cerr << "Warn: analysis config 'pid(" << optarg <<
                    ")' value must be a non-negative integer.\n";
                    PrintHelp();
                    return false;
                }
                break;
            case NUMA_THREAD_THRESHOLD:
                if (!oeaware::IsInteger(optarg)) {
                    std::cerr << "Error: Invalid numa thread threshold: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                numaThreadThreshold = atoi(optarg);
                if (numaThreadThreshold < 0) {
                    std::cerr << "Warn: analysis config 'numa_analysis:numaThreadThreshold(" << optarg <<
                    ")' value must be a non-negative integer.\n";
                    PrintHelp();
                    return false;
                }
                numaThreadThresholdSet = true;
                break;
            case SMC_CHANGE_RATE:
                if (!oeaware::IsNum(optarg)) {
                    std::cerr << "Error: Invalid smc change rate: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                smcChangeRate = atof(optarg);
                if (smcChangeRate < 0) {
                    std::cerr << "Warn: analysis config 'smc_d_analysis:smcChangeRate(" << optarg <<
                    ")' value must be  non-negative integer.\n";
                    PrintHelp();
                    return false;
                }
                smcChangeRateSet = true;
                break;
            case SMC_LONET_FLOW:
                if (!oeaware::IsNum(optarg)) {
                    std::cerr << "Error: Invalid smc local net flow: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                smcLoNetFlow = atoi(optarg);
                 if (smcLoNetFlow < 0) {
                    std::cerr << "Warn: analysis config 'smc_d_analysis:smcLoNetFlow(" << optarg <<
                    ")' value must be  non-negative integer.\n";
                    PrintHelp();
                    return false;
                }
                smcLoNetFlowSet = true;
                break;
            case HOST_CPU_USAGE_THRESHOLD:
                if (!oeaware::IsNum(optarg)) {
                    std::cerr << "Error: Invalid host-cpu-usage-threshold: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                hostCpuUsageThreshold = atof(optarg);
                if (hostCpuUsageThreshold < 0 || hostCpuUsageThreshold > THRESHOLD_UP) {
                    std::cerr << "Warn: analysis config 'docker_coordination_burst:host_cpu_usage_threshold(" <<
                    optarg << ")' value must be a [0, 100].\n";
                    PrintHelp();
                    return false;
                }
                hostCpuUsageThresholdSet = true;
                break;
            case DOCKER_CPU_USAGE_THRESHOLD:
                if (!oeaware::IsNum(optarg)) {
                    std::cerr << "Error: Invalid docker-cpu-usage-threshold: '" << optarg << "'\n";
                    PrintHelp();
                    return false;
                }
                dockerCpuUsageThreshold = atof(optarg);
                if (dockerCpuUsageThreshold < 0 || dockerCpuUsageThreshold > THRESHOLD_UP) {
                    std::cerr << "Warn: analysis config 'docker_coordination_burst:dockerCpuUsageThreshold(" <<
                    optarg << ")' value must be a [0, 100].\n";
                    PrintHelp();
                    return false;
                }
                dockerCpuUsageThresholdSet = true;
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

void Config::DockerCoordinationBurstConfig(const YAML::Node config)
{
    if (!config["docker_coordination_burst"]) {
        return;
    }
    auto dockerCoordinationBurstConfig = config["docker_coordination_burst"];
    double threshold = 0;
    if (dockerCoordinationBurstConfig["host_cpu_usage_threshold"]) {
        std::string cooString = dockerCoordinationBurstConfig["host_cpu_usage_threshold"].as<std::string>();
        if (!oeaware::IsNum(cooString)) {
            std::cerr << "Warn: analysis config 'docker_coordination_burst:host_cpu_usage_threshold(" << cooString <<
            ")' value must be a number.\n";
        } else {
            threshold = dockerCoordinationBurstConfig["host_cpu_usage_threshold"].as<double>();
            if (threshold < 0 || threshold > THRESHOLD_UP) {
                std::cerr << "Warn: analysis config 'docker_coordination_burst:host_cpu_usage_threshold(" <<
                threshold << ")' value must be a [0, 100].\n";
            } else if (!hostCpuUsageThresholdSet) {
                hostCpuUsageThreshold = threshold;
            }
        }
    }

    if (dockerCoordinationBurstConfig["docker_cpu_usage_threshold"]) {
        std::string cpuUsageString = dockerCoordinationBurstConfig["docker_cpu_usage_threshold"].as<std::string>();
        if (!oeaware::IsNum(cpuUsageString)) {
            std::cerr << "Warn: analysis config 'docker_coordination_burst:docker_cpu_usage_threshold(" <<
            cpuUsageString << ")' value must be a number.\n";
        } else {
            threshold = dockerCoordinationBurstConfig["docker_cpu_usage_threshold"].as<double>();
            if (threshold < 0 || threshold > THRESHOLD_UP) {
                std::cerr << "Warn: analysis config 'docker_coordination_burst:docker_cpu_usage_threshold(" <<
                threshold << ")' value must be [0, 100].\n";
            } else if (!dockerCpuUsageThresholdSet) {
                dockerCpuUsageThreshold = threshold;
            }
        }
    }
}

void Config::LoadMicroArchTidNoCmpConfig(const YAML::Node config)
{
    if (!config["microarch_tidnocmp"]) {
        return ;
    }
    auto microArchTidNoCmpConfig = config["microarch_tidnocmp"];
    std::stringstream yamlStream;
    yamlStream << config["microarch_tidnocmp"];  // Output node content to stream, then parsed by the plugin
    microArchTidNoCmpConfigStream = yamlStream.str();
    return ;
}

void Config::LoadDynamicSmtConfig(const YAML::Node &config)
{
    if (!config["dynamic_smt"]) {
        return;
    }
    auto dynamic_smt = config["dynamic_smt"];
    double threshold = -1;
    if (dynamic_smt["threshold"]) {
        std::string thresholdString = dynamic_smt["threshold"].as<std::string>();
        if (!oeaware::IsNum(thresholdString)) {
            std::cerr << "Warn: analysis config 'dynamic_smt:threshold(" << thresholdString <<
                ")' value must be a number.\n";
            return;
        } else {
            threshold = dynamic_smt["threshold"].as<double>();
        }
    }
    if (threshold < 0 || threshold > THRESHOLD_UP) {
        std::cerr << "Warn: analysis config 'dynamic_smt:threshold(" << threshold <<
            ")' value must be [0, 100].\n";
    } else if (!dynamicSmtThresholdSet) {
        dynamicSmtThreshold = threshold;
    }
}

void Config::LoadHugepageConfig(const YAML::Node &config)
{
    if (!config["hugepage"]) {
        return;
    }
    auto hugepage = config["hugepage"];
    double l1 = -1;
    double l2 = -1;
    if (hugepage["l1_miss_threshold"]) {
        std::string l1String = hugepage["l1_miss_threshold"].as<std::string>();
        if (!oeaware::IsNum(l1String)) {
            std::cerr << "Warn: analysis config 'hugepage:l1_miss_threshold(" << l1String <<
                ")' value must be a number.\n";
        } else {
            l1 = hugepage["l1_miss_threshold"].as<double>();
            if (l1 < 0 || l1 > THRESHOLD_UP) {
                std::cerr << "Warn: analysis config 'hugepage:l1_miss_threshold(" << l1String <<
                    ")' value must be [0, 100].\n";
            } else if (!l1MissThresholdSet) {
                l1MissThreshold = l1;
            }
        }
    }
    if (hugepage["l2_miss_threshold"]) {
        std::string l2String = hugepage["l2_miss_threshold"].as<std::string>();
        if (!oeaware::IsNum(hugepage["l2_miss_threshold"].as<std::string>())) {
            std::cerr << "Warn: analysis config 'hugepage:l2_miss_threshold(" << l2String <<
            ")' value must be a number.\n";
        } else {
            l2 = hugepage["l2_miss_threshold"].as<double>();
            if (l2 < 0 || l2 > THRESHOLD_UP) {
                std::cerr << "Warn: analysis config 'hugepage:l1_miss_threshold(" << l2String <<
                    ")' value must be [0, 100].\n";
            } else if (l2MissThresholdSet) {
                l2MissThreshold = l2;
            }
        }
    }
}

void Config::LoadNumaConfig(const YAML::Node &config)
{
    if (!config["numa_analysis"]) {
        return;
    }
    auto numa_analysis = config["numa_analysis"];
    int threshold = -1;
    if (numa_analysis["thread_threshold"]) {
        std::string thresholdString = numa_analysis["thread_threshold"].as<std::string>();
        if (!oeaware::IsInteger(thresholdString)) {
            std::cerr << "Warn: analysis config 'numa_analysis:thread_threshold(" << thresholdString <<
                ")' value must be a integer.\n";
            return;
        } else {
            threshold = numa_analysis["thread_threshold"].as<int>();
        }
    }
    if (threshold < 0) {
        std::cerr << "Warn: analysis config 'numa_analysis:thread_threshold(" << threshold <<
            ")' value must be a non-negative integer.\n";
        return;
    }
    if (!numaThreadThresholdSet) {
        numaThreadThreshold = threshold;
    }
}

void Config::LoadSmcDConfig(const YAML::Node &config)
{
    if (!config["smc_d_analysis"]) {
        return;
    }
    auto smc_d_analysis = config["smc_d_analysis"];
    double changeRate = -1;
    double loNetFlow = -1;
    if (smc_d_analysis["change_rate"]) {
        std::string rateString = smc_d_analysis["change_rate"].as<std::string>();
        if (!oeaware::IsNum(rateString)) {
            std::cerr << "Warn: analysis config 'smc_d_analysis:change_rate(" << rateString <<
                ")' value must be a number.\n";
        } else {
            changeRate = smc_d_analysis["change_rate"].as<double>();
            if (changeRate < 0) {
                std::cerr << "Warn: analysis config 'smc_d_analysis:change_rate(" << changeRate <<
                    ")' value must a non-negative number.\n";
            } else if (!smcChangeRateSet) {
                smcChangeRate = changeRate;
            }
        }
    }
    if (smc_d_analysis["local_net_flow"]) {
        std::string flowString = smc_d_analysis["local_net_flow"].as<std::string>();
        if (!oeaware::IsNum(flowString)) {
            std::cerr << "Warn: analysis config 'smc_d_analysis:local_net_flow(" << flowString <<
                ")' value must be a number.\n";
        } else {
            loNetFlow = smc_d_analysis["local_net_flow"].as<double>();
            if (loNetFlow < 0) {
                std::cerr << "Warn: analysis config 'smc_d_analysis:local_net_flow(" << loNetFlow <<
                    ")' value must a non-negative number.\n";
            } else if (!smcLoNetFlowSet) {
                smcLoNetFlow = loNetFlow;
            }
        }
    }
}

void Config::LoadXcallConfig(const YAML::Node &config)
{
    if (!config["xcall_analysis"]) {
        return;
    }
    auto xcallAnalysis = config["xcall_analysis"];
    double threshold = -1;
    int num = -1;
    if (xcallAnalysis["threshold"]) {
        std::string thString = xcallAnalysis["threshold"].as<std::string>();
        if (!oeaware::IsNum(thString)) {
            std::cerr << "Warn: analysis config 'xcall_analysis:threshold(" << thString <<
                ")' value must be a number.\n";
        } else {
            threshold = xcallAnalysis["threshold"].as<double>();
            if (threshold < 0 || threshold > THRESHOLD_UP) {
                std::cerr << "Warn: analysis config 'xcall_analysis:threshold(" << thString <<
                    ")' value must be [0, 100].\n";
            } else {
                xcallThreshold = threshold;
            }
        }
    }
    if (xcallAnalysis["num"]) {
        std::string numString = xcallAnalysis["num"].as<std::string>();
        if (!oeaware::IsInteger(numString)) {
            std::cerr << "Warn: analysis config 'xcall_analysis:num(" << numString <<
                ")' value must be a integer.\n";
        } else {
            num = xcallAnalysis["num"].as<int>();
            if (num < 0) {
                std::cerr << "Warn: analysis config 'xcall_analysis:num(" << numString <<
                    ")' value must be a non-negative integer.\n";
            } else {
                xcallTopNum = num;
            }
        }
    }
}

bool Config::LoadConfig(const std::string& configPath)
{
    try {
        YAML::Node config = YAML::LoadFile(configPath);
        LoadDynamicSmtConfig(config);
        LoadHugepageConfig(config);
        LoadNumaConfig(config);
        LoadSmcDConfig(config);
        LoadXcallConfig(config);
        DockerCoordinationBurstConfig(config);
        LoadMicroArchTidNoCmpConfig(config);
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
}

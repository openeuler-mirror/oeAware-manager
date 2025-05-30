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
#include <yaml-cpp/yaml.h>

const int L1_MISS_THRESHOLD = 200;
const int L2_MISS_THRESHOLD = 201;
const int OUT_PATH = 202;
const int DYNAMIC_SMT_THRESHOLD = 203;
const int NUMA_THREAD_THRESHOLD = 204;
const int PID = 205;
const int SMC_CHANGE_RATE = 206 ;
const int SMC_LONET_FLOW = 207;
const int HOST_CPU_USAGE_THRESHOLD = 208;
const int DOCKER_CPU_USAGE_THRESHOLD = 209;

constexpr double HOST_CPU_USAGE_THRESHOLD_DEFAULT = 45.0;
constexpr double DOCKER_CPU_USAGE_THRESHOLD_DEFAULT = 95.0;

const int THRESHOLD_UP = 100;
class Config {
public:
    bool Init(int argc, char **argv);
    bool LoadConfig(const std::string& configPath);
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
    double GetDynamicSmtThreshold() const
    {
        return dynamicSmtThreshold;
    }
    int GetPid() const
    {
        return pid;
    }
    int GetNumaThreadThreshold() const
    {
        return numaThreadThreshold;
    }
    double GetSmcChangeRate() const
    {
        return smcChangeRate;
    }
    int GetSmcLoNetFlow() const
    {
        return smcLoNetFlow;
    }
    double GetDockerCoordinationBurstHostCpuUsageThreshold() const
    {
        return hostCpuUsageThreshold;
    }
    double GetDockerCoordinationBurstDockerCpuUsageThreshold() const
    {
        return dockerCpuUsageThreshold;
    }
    std::string GetMicroArchTidNoCmpConfigStream() const
    {
        return microArchTidNoCmpConfigStream;
    }
    double GetXcallThreshold() const
    {
        return xcallThreshold;
    }
    int GetXcallTopNum() const
    {
        return xcallTopNum;
    }
private:
    const int minAnalyzeTime = 1;
    const int maxAnalyzeTime = 100;
    int analysisTime = 30; // default 30s
    int pid = -1;
    double l1MissThreshold = 5;
    double l2MissThreshold = 10;
    double dynamicSmtThreshold = 40;
    std::string  outMarkDownPath = "";
    int numaThreadThreshold = 200; // defaut 200 threads
    double smcChangeRate = 0.1;
    int smcLoNetFlow = 100; // default 100MB/S
    double xcallThreshold = 5;
    int xcallTopNum = 5;
    double hostCpuUsageThreshold = HOST_CPU_USAGE_THRESHOLD_DEFAULT;
    double dockerCpuUsageThreshold = DOCKER_CPU_USAGE_THRESHOLD_DEFAULT;
    std::string microArchTidNoCmpConfigStream = "";
    const std::string shortOptions = "t:hrv";
    const std::vector<option> longOptions = {
        {"help", no_argument, nullptr, 'h'},
        {"realtime", no_argument, nullptr, 'r'},
        {"time", required_argument, nullptr, 't'},
        {"verbose", no_argument, nullptr, 'v'},
        {"l1-miss-threshold", required_argument, nullptr, L1_MISS_THRESHOLD},
        {"l2-miss-threshold", required_argument, nullptr, L2_MISS_THRESHOLD},
        {"dynamic-smt-threshold", required_argument, nullptr, DYNAMIC_SMT_THRESHOLD},
        {"out-path", required_argument, nullptr, OUT_PATH},
        {"pid", required_argument, nullptr, PID},
        {"numa-thread-threshold", required_argument, nullptr, NUMA_THREAD_THRESHOLD},
        {"smc-change-rate", required_argument, nullptr, SMC_CHANGE_RATE},
        {"smc-localnet-flow", required_argument, nullptr, SMC_LONET_FLOW},
        {"host-cpu-usage-threshold", required_argument, nullptr, HOST_CPU_USAGE_THRESHOLD},
        {"docker-cpu-usage-threshold", required_argument, nullptr, DOCKER_CPU_USAGE_THRESHOLD},
        {nullptr, 0, nullptr, 0}
    };
    bool isShowRealTimeReport = false;
    bool showVerbose = false;
    bool l1MissThresholdSet = false;
    bool l2MissThresholdSet = false;
    bool dynamicSmtThresholdSet = false;
    bool numaThreadThresholdSet = false;
    bool smcChangeRateSet = false;
    bool smcLoNetFlowSet = false;
    bool hostCpuUsageThresholdSet = false;
    bool dockerCpuUsageThresholdSet = false;
    bool supporCpuPartIdSet = false;
    bool keyServiceListSet = false;
    bool ParseTime(const char *arg);
    void PrintHelp();
    void DockerCoordinationBurstConfig(const YAML::Node config);
    void LoadMicroArchTidNoCmpConfig(const YAML::Node config);
    void LoadDynamicSmtConfig(const YAML::Node &config);
    void LoadHugepageConfig(const YAML::Node &config);
    void LoadNumaConfig(const YAML::Node &config);
    void LoadSmcDConfig(const YAML::Node &config);
    void LoadXcallConfig(const YAML::Node &config);
};

#endif
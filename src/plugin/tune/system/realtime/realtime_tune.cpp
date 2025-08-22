/******************************************************************************
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
* oeAware is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
******************************************************************************/
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <yaml-cpp/yaml.h>
#include "oeaware/utils.h"
#include "realtime_tune.h"

using namespace oeaware;
RealTimeTune::RealTimeTune()
{
	name = OE_REALTIME_TUNE;
	description = "To enable realtime";
	version = "1.0.0";
	period = defaultPeriod; // 1000 period is meaningless, only Enable() is executed, not Run()
	priority = defaultPriority;
	type = oeaware::TUNE;
}

oeaware::Result RealTimeTune::OpenTopic(const oeaware::Topic &topic)
{
	(void)topic;
	return oeaware::Result(OK);
}

void RealTimeTune::CloseTopic(const oeaware::Topic &topic)
{
	(void)topic;
}

void RealTimeTune::UpdateData(const DataList &dataList)
{
	(void)dataList;
}

oeaware::Result RealTimeTune::Enable(const std::string &param)
{
    (void)param;
    if(!CheckEnv()) return oeaware::Result(FAILED, "Current kernel is not PREEMPT_RT! yum install kernel-rt first.");
    UpdateMaxCpuId();
    if (!ParseFromSwitchConfig()) return oeaware::Result(FAILED, "read realtime_tune.yaml failed"); // 从配置文件中读取开关状态
    if(!AddRealtimeTuneToConfig()) return oeaware::Result(FAILED, "add realtime_tune to config failed"); // 将realtime_tune默认启动添加到配置文件中
    if (!switchConfig.cpuIsolationRange.empty() && switchConfig.cpuIsolationRange != "0") {
        if (!CheckAndUpdateKernelParamsWithGrubby()) return oeaware::Result(FAILED, "check and update grubby params failed"); // 检查grubby参数，如果与预期不符，则更新
    } else {
        INFO(logger, "cpu_isolation.range is 0 or empty, skip check and update grubby params.");
    }
    bool ok = OpenProcAndSys(); // proc和sys项配置，如果配置项不存在，则跳过
    if(!AddRealtimeTuneToConfig()) return oeaware::Result(FAILED, "add realtime_tune to config failed"); // 将realtime_tune默认启动添加到配置文件中
    if (ok) {
        return oeaware::Result(OK, "Realtime enabled successfully,all items are enabled,please reboot for changes to take effect");
    } else {
        return oeaware::Result(OK, "Realtime enabled successfully, but some items skiped,please reboot for changes to take effect");
    }
}

void RealTimeTune::Disable()
{
    DisableProcAndSys();
     if (isRebooting) {
        // 正常关闭，参数变更需要重启，此时不移除cmdline参数
        INFO(logger, "Kernel parameter changes require reboot, skip removing kernel params in disable.");
        isRebooting = false;
    } else {
        // 重启后，cmdline已生效，可以正常移除
        bool needRemove = false;
        for (const auto& kv : switchConfig.featureSwitches) {
            if (kv.second == true) {
                needRemove = true;
                break;
            }
        }
        if (!switchConfig.cpuIsolationRange.empty() && switchConfig.cpuIsolationRange != "0" && needRemove) {
            DisableKernelParams();
            int ret = system("grub2-mkconfig -o /boot/grub2/grub.cfg");
            if (ret != 0) {
                ERROR(logger, "Failed to update grub config.");
            } else {
                INFO(logger, "Grub config updated successfully.");
                INFO(logger, "Kernel params removed, please reboot for changes to take effect.");
            }
        } else {
            INFO(logger, "cmdline do not need to recover.");
        }
        RemoveRealtimeTuneFromConfig();
    }
}

void RealTimeTune::DisableProcAndSys()
{
    // 恢复 swap
    if (switchConfig.swap) {
        std::string result;
        if (!oeaware::ExecCommand(swaponCmd, result)) {
            ERROR(logger, "Failed to execute command: " + swaponCmd);
        }
    }
    // 恢复 KSM
    if (switchConfig.ksm && backup.ksmRun != -1) {
        SetStringValue(ksmRunPath, std::to_string(backup.ksmRun));
    }
    // 恢复 NUMA balancing
    if (switchConfig.numaBalancing && backup.numaBalancing != -1) {
        SetStringValue(numaBalancingPath, std::to_string(backup.numaBalancing));
    }
    // 恢复 timer_migration
    if (switchConfig.timerMigration && backup.timerMigration != -1) {
        SetStringValue(timerMigrationPath, std::to_string(backup.timerMigration));
    }
    // 恢复 sched_rt_runtime_us
    if (switchConfig.rtRuntimeUs == -1 && backup.schedRtRuntime != -1) {
        SetStringValue(schedRuntimePath, std::to_string(backup.schedRtRuntime));
    }
    // 恢复透明大页
    if (switchConfig.transparentHugepage && !backup.thpEnabled.empty()) {
        SetStringValue(thpEnabledPath, backup.thpEnabled);
    }
    // 恢复每个cpu的governor策略
    if (switchConfig.cpufreqPerformance) {
        for (const auto& kv : backup.cpuGovs) {
            if (kv.second != "performance") {
                SetStringValue(kv.first, kv.second);
            }
        }
    }
}

void RealTimeTune::Run()
{
}

void RealTimeTune::UpdateMaxCpuId()
{
    maxCpuId = sysconf(_SC_NPROCESSORS_CONF) - 1;
}

int RealTimeTune::GetMaxCpuId()
{
    return maxCpuId;
}

bool RealTimeTune::GetInitialValue(const std::string& path, long& value)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        ERROR(logger, "Failed to open file: " + path);
        return false;
    }

    std::string line;
    if (!std::getline(file, line)) {
        ERROR(logger, "Failed to read from file: " + path);
        file.close();
        return false;
    }
    file.close();

    std::istringstream iss(line);
    int parsedValue;
    if (!(iss >> parsedValue)) {
        ERROR(logger, "Failed to parse value from file: " + path);
        return false;
    }
    value = parsedValue;
    return true;
}

bool RealTimeTune::SetStringValue(const std::string& path, const std::string& value)
{
    std::ofstream file(path);
    if (!file.is_open()) {
        ERROR(logger, "Failed to open file for writing: " + path);
        return false;
    }
    file << value;
    if (!file.good()) {
        ERROR(logger, "Failed to write to file: " + path);
        file.close();
        return false;
    }
    file.flush();
    file.close();
    return true;
}

bool RealTimeTune::OpenProcAndSys() // 打开proc和sys项配置,不需要重启
{
    bool cpuFreqOk = true;
    bool thpOk = true;
    bool numaOk = true;
    bool ksmOk = true;
    bool swapoffOk = true;
    bool setDefaultTimerMigration = true;
    bool setDefaultSchedRuntime = true;

    if (switchConfig.cpufreqPerformance) {
        cpuFreqOk = SetCpuFreqPerformance();
        if (!cpuFreqOk) {
            WARN(logger, "The config item cpufreq_performance does not exist in this system, skip.");
        }
    }
    if (switchConfig.transparentHugepage) {
        backup.thpEnabled = GetCurrentThpMode();
        thpOk = SetStringValue(thpEnabledPath, "never");
        if (!thpOk) {
            WARN(logger, "The config item transparent_hugepage does not exist in this system, skip.");
        }
    }
    if (switchConfig.numaBalancing) {
        GetInitialValue(numaBalancingPath, backup.numaBalancing);
        numaOk = SetStringValue(numaBalancingPath, "0");
        if (!numaOk) {
            WARN(logger, "The config item numa_balancing does not exist in this system, skip.");
        }
    }
    if (switchConfig.ksm) {
        GetInitialValue(ksmRunPath, backup.ksmRun);
        ksmOk = SetStringValue(ksmRunPath, "0");
        if (!ksmOk) {
            WARN(logger, "The config item ksm does not exist in this system, skip.");
        }
    }
    if (switchConfig.swap) {
        std::string result;
        swapoffOk = oeaware::ExecCommand(swapoffCmd, result);
        if (!swapoffOk) {
            WARN(logger, "Failed to execute command: " + swapoffCmd);
        }
    }
    if (switchConfig.timerMigration) {
        GetInitialValue(timerMigrationPath, backup.timerMigration);
        setDefaultTimerMigration = SetStringValue(timerMigrationPath, "0");
        if (!setDefaultTimerMigration) {
            WARN(logger, "The config item timer.migration does not exist in this system, skip.");
        }
    }
    if (switchConfig.rtRuntimeUs == -1) {
        GetInitialValue(schedRuntimePath, backup.schedRtRuntime);
        setDefaultSchedRuntime = SetStringValue(schedRuntimePath, "-1");
        if (!setDefaultSchedRuntime) {
            WARN(logger, "The config item sched.rt_runtime_us does not exist in this system, skip.");
        }
    }

    return cpuFreqOk && thpOk && numaOk && ksmOk && swapoffOk && setDefaultTimerMigration && setDefaultSchedRuntime;
}

bool RealTimeTune::SetCpuFreqPerformance()
{
    bool allOk = true;
    DIR* dir = opendir(cpuFreqPath.c_str());
    if (!dir) {
        ERROR(logger, "Failed to open cpu directory: " + cpuFreqPath);
        return false;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // 只处理cpu开头的目录且后面是数字
        if (strncmp(entry->d_name, "cpu", 3) != 0 || !isdigit(entry->d_name[3]))
            continue;
        std::string govPath = cpuFreqPath + entry->d_name + "/cpufreq/scaling_governor";
        struct stat st;
        if (stat(govPath.c_str(), &st) != 0)
            continue; // 文件不存在
        std::ifstream govFile(govPath);
        if (!govFile.is_open())
            continue;
        std::string curGov;
        std::getline(govFile, curGov);
        govFile.close();
        // 先保存原始策略
        backup.cpuGovs[govPath] = curGov;
        if (curGov != "performance") {
            bool ok = SetStringValue(govPath, "performance");
            INFO(logger, "Initial curGov: " + curGov);
            if (!ok) {
                ERROR(logger, "Failed to set " + govPath + " to performance");
                allOk = false;
            }
        }
    }
    closedir(dir);
    return allOk;
}

std::string RealTimeTune::GetCurrentThpMode()
{
    std::ifstream thpFile(thpEnabledPath);
    if (thpFile.is_open()) {
        std::string modes;
        std::getline(thpFile, modes);
        thpFile.close();
        size_t startPos = modes.find('[') + 1;
        size_t endPos = modes.find(']', startPos);
        if (startPos != std::string::npos && endPos != std::string::npos) {
            return modes.substr(startPos, endPos - startPos);
        }
    }
    ERROR(logger, "Failed to read current THP mode.");
    return "";
}

bool RealTimeTune::CheckEnv()
{
    std::string kernel;
    if (!oeaware::ExecCommand("uname -v", kernel)) {
        ERROR(logger, "Failed to run uname -v");
        return false;
    }
    
    // 去除换行符
    if (!kernel.empty() && kernel[kernel.size() - 1] == '\n') {
        kernel.erase(kernel.size() - 1);
    }
    
    INFO(logger, "Current kernel: " + kernel);
    if (kernel.find("PREEMPT_RT") != std::string::npos) {
        return true;
    } else {
        ERROR(logger, "Current kernel is not PREEMPT_RT!");
        return false;
    }
}

bool RealTimeTune::AddRealtimeTuneToConfig(const std::string& configPath)
{
    try {
        YAML::Node config;
        try {
            config = YAML::LoadFile(configPath);
        } catch (const std::exception& e) {
            ERROR(logger, "Failed to load config: " + std::string(e.what()));
            return false;
        }

        // 获取 enable_list（不存在则创建）
        YAML::Node enable_list;
        if (config["enable_list"] && config["enable_list"].IsSequence()) {
            enable_list = config["enable_list"];
        } else {
            enable_list = YAML::Node(YAML::NodeType::Sequence);
        }

        bool foundSystemTune = false;
        bool alreadyAdded = false;

        // 遍历 enable_list
        for (const auto& item : enable_list) {
            if (item["name"] && item["name"].as<std::string>() == "libsystem_tune.so") {
                foundSystemTune = true;
                    // 检查是否有 instances 字段
                    YAML::Node instances = item["instances"];
                    if (!instances.IsDefined() || instances.IsNull()) {
                        // 没有 instances 字段，表示系统会启动所有实例，包括 realtime_tune
                        INFO(logger, "libsystem_tune.so has no instances field, will start all instances including realtime_tune.");
                        return true;
                    } else {
                        if (!instances.IsSequence()) {
                            WARN(logger, "the format of the enable list{libsystem_tune.so} is incorrect.");
                            return false;
                        }
                        // 有 instances 字段，检查是否已包含 realtime_tune
                        for (const auto& inst : instances) {
                            if (inst.as<std::string>() == "realtime_tune") {
                                INFO(logger, "realtime_tune already in libsystem_tune.so instances.");
                                alreadyAdded = true;
                                break;
                            }
                        }
                        if (!alreadyAdded) {
                            instances.push_back("realtime_tune");
                            ((YAML::Node&)item)["instances"] = instances;
                            INFO(logger, "Added realtime_tune to libsystem_tune.so instances.");
                        }
                    }
                break;  // 已处理 libsystem_tune.so，跳出循环
            }
        }

        // 没有找到 libsystem_tune.so，新增一个
        if (!foundSystemTune) {
            YAML::Node newItem;
            newItem["name"] = "libsystem_tune.so";
            YAML::Node instList(YAML::NodeType::Sequence);
            instList.push_back("realtime_tune");
            newItem["instances"] = instList;
            enable_list.push_back(newItem);
            INFO(logger, "Added libsystem_tune.so with realtime_tune instance.");
        }

        // 写回配置
        config["enable_list"] = enable_list;

        std::ofstream fout(configPath);
        fout << config;
        fout.close();

        return true;
        
    } catch (const std::exception& e) {
        ERROR(logger, "AddRealtimeTuneToConfig failed with exception: " + std::string(e.what()));
        return false;
    }
}

bool RealTimeTune::RemoveRealtimeTuneFromConfig(const std::string& configPath)
{
    YAML::Node config;
    try {
        config = YAML::LoadFile(configPath);
    } catch (const std::exception& e) {
        ERROR(logger, "Failed to load config: " + std::string(e.what()));
        return false;
    }

    if (!config["enable_list"] || !config["enable_list"].IsSequence()) {
        INFO(logger, "enable_list not found or not a sequence.");
        return true;
    }

    YAML::Node newList(YAML::NodeType::Sequence);
    for (const auto& item : config["enable_list"]) {
        if (item.IsMap() && item["name"] && item["name"].as<std::string>() == "libsystem_tune.so") {
            // 只处理 libsystem_tune.so
            if (item["instances"] && item["instances"].IsSequence()) {
                YAML::Node newInstances(YAML::NodeType::Sequence);
                int otherCount = 0;
                for (const auto& inst : item["instances"]) {
                    if (inst.as<std::string>() != "realtime_tune") {
                        newInstances.push_back(inst);
                        ++otherCount;
                    }
                }
                if (otherCount > 0) {
                    // 还有其它实例，保留 libsystem_tune.so，只去掉 realtime_tune
                    YAML::Node newItem = item;
                    newItem["instances"] = newInstances;
                    newList.push_back(newItem);
                }
            } else {
                // 没有 instances 字段，表示全部启用，不处理，直接返回
                return true;
            }
        } else {
            // 不是 libsystem_tune.so，原样保留
            newList.push_back(item);
        }
    }
    // 如果 newList 为空，输出 enable_list:（null，不留方括号）
    if (newList.size() > 0) {
        config["enable_list"] = newList;
    } else {
        config["enable_list"] = YAML::Node();
    }

    try {
        std::ofstream fout(configPath);
        fout << config;
        fout.close();
    } catch (const std::exception& e) {
        ERROR(logger, "Failed to write config: " + std::string(e.what()));
        return false;
    }

    INFO(logger, "Removed realtime_tune from enable_list.");
    return true;
}

bool IsValidSwitch(const std::string& val)
{
    return val == "on" || val == "off";
}


bool RealTimeTune::ParseFromSwitchConfig(const std::string& configPath)
{
    try {
        YAML::Node config = YAML::LoadFile(configPath);

        // 解析失败，返回false

        if (!ParseCpuIsolationConfig(config)) {
            return false; 
        }
        
        if (!ParseCpuFreqConfig(config)) {
            return false;
        }
        
        if (!ParseMemoryConfig(config)) {
            return false;
        }
        
        if (!ParseTimerConfig(config)) {
            return false;
        }
        
        if (!ParseSchedConfig(config)) {
            return false;
        }
        
    } catch (const std::exception& e) {
        ERROR(logger, "Failed to load configuration file: " + std::string(e.what()));
        return false;
    }
    return true;
}

bool RealTimeTune::ParseCpuIsolationConfig(const YAML::Node& config)
{
    if (!config["cpu_isolation"]) {
        ERROR(logger, "cpu_isolation missing or not map");
        return false;
    }
    
    auto cpuIso = config["cpu_isolation"];
    
    // 解析 range
    if (!ParseCpuIsolationRange(cpuIso)) {
        return false;
    }
    
    // 解析 features
    if (!ParseCpuIsolationFeatures(cpuIso)) {
        return false;
    }
    
    return true;
}

bool RealTimeTune::ParseCpuIsolationRange(const YAML::Node& cpuIso)
{
    if (!cpuIso["range"] || !cpuIso["range"].IsScalar()) {
        ERROR(logger, "cpu_isolation.range missing or not scalar");
        return false;
    }
    
    std::string rangeValue = cpuIso["range"].as<std::string>();
    
    if (!ValidateCpuRange(rangeValue)) {
        return false;
    }
    
    switchConfig.cpuIsolationRange = rangeValue;
    INFO(logger, "cpu_isolation.range = " + switchConfig.cpuIsolationRange);
    return true;
}

bool RealTimeTune::ValidateCpuRange(const std::string& rangeValue)
{
    if (rangeValue.length() > 256) {
        ERROR(logger, "cpu_isolation.range too long: " + rangeValue);
        return false;
    }
    
    // 检查格式是否合法（只允许数字、逗号、连字符）
    std::regex validRangePattern("^[0-9,\\-\\s]+$");
    if (!std::regex_match(rangeValue, validRangePattern)) {
        ERROR(logger, "cpu_isolation.range contains invalid characters: " + rangeValue);
        return false;
    }
    
    // 检查数字范围和格式
    std::stringstream ss(rangeValue);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);
        
        if (token.empty()) {
            ERROR(logger, "cpu_isolation.range contains empty token");
            return false;
        }
        
        if (!ValidateCpuToken(token)) {
            return false;
        }
    }
    
    return true;
}

bool RealTimeTune::ValidateCpuToken(const std::string& token)
{
    size_t dash = token.find('-');
    if (dash != std::string::npos) {
        // 格式: 数字-数字
        std::string startStr = token.substr(0, dash);
        std::string endStr = token.substr(dash + 1);
        
        // 检查是否为纯数字
        if (startStr.empty() || endStr.empty() || 
            startStr.find_first_not_of("0123456789") != std::string::npos ||
            endStr.find_first_not_of("0123456789") != std::string::npos) {
            ERROR(logger, "Invalid CPU range format: " + token);
            return false;
        }
        
        int start = std::stoi(startStr);
        int end = std::stoi(endStr);
        if (start < 0 || end > maxCpuId || start > end) {
            ERROR(logger, "Invalid CPU range: " + token + " (max CPU: " + std::to_string(maxCpuId) + ")");
            return false;
        }
    } else {
        // 格式: 数字
        if (token.find_first_not_of("0123456789") != std::string::npos) {
            ERROR(logger, "Invalid CPU ID format: " + token);
            return false;
        }
        int cpuId = std::stoi(token);
        if (cpuId < 0 || cpuId > maxCpuId) {
            ERROR(logger, "Invalid CPU ID: " + token + " (max CPU: " + std::to_string(maxCpuId) + ")");
            return false;
        }
    }
    
    return true;
}

bool RealTimeTune::ParseCpuIsolationFeatures(const YAML::Node& cpuIso)
{
    if (!cpuIso["features"] || !cpuIso["features"].IsMap()) {
        ERROR(logger, "cpu_isolation.features missing or not map");
        return false;
    }
    
    auto features = cpuIso["features"];
    for (const auto& key : KERNEL_PARAM_KEYS) {
        if (!features[key] || !features[key].IsScalar()) {
            ERROR(logger, "cpu_isolation.features." + key + " missing or not scalar");
            return false;
        }
        
        std::string val = features[key].as<std::string>();
        if (!IsValidSwitch(val)) {
            ERROR(logger, "Invalid cpu_isolation.features." + key + " value: " + val);
            return false;
        }
        
        switchConfig.featureSwitches[key] = (val == "on");
    }
    
    return true;
}

bool RealTimeTune::ParseCpuFreqConfig(const YAML::Node& config)
{
    if (!config["cpufreq_performance"]) {
        ERROR(logger, "cpufreq_performance missing");
        return false;
    }
    
    if (!config["cpufreq_performance"].IsScalar()) {
        ERROR(logger, "cpufreq_performance must be scalar (on/off)");
        return false;
    }
    
    std::string val = config["cpufreq_performance"].as<std::string>();
    if (!IsValidSwitch(val)) {
        ERROR(logger, "Invalid cpufreq_performance value: " + val);
        return false;
    }
    
    switchConfig.cpufreqPerformance = (val == "on");
    return true;
}

bool RealTimeTune::ParseMemoryConfig(const YAML::Node& config)
{
    if (!config["memory"]) {
        ERROR(logger, "memory missing or not map");
        return false;
    }
    
    auto mem = config["memory"];
    
    if (!ParseMemorySwitch(mem, "transparent_hugepage", switchConfig.transparentHugepage, true)) {
        return false;
    }
    
    if (!ParseMemorySwitch(mem, "numa_balancing", switchConfig.numaBalancing, true)) {
        return false;
    }
    
    if (!ParseMemorySwitch(mem, "ksm", switchConfig.ksm, true)) {
        return false;
    }
    
    if (!ParseMemorySwitch(mem, "swap", switchConfig.swap, true)) {
        return false;
    }
    
    return true;
}

bool RealTimeTune::ParseMemorySwitch(const YAML::Node& mem, const std::string& key, bool& target, bool invert)
{
    if (!mem[key] || !mem[key].IsScalar()) {
        ERROR(logger, "memory." + key + " missing or not scalar");
        return false;
    }
    
    std::string val = mem[key].as<std::string>();
    if (!IsValidSwitch(val)) {
        ERROR(logger, "Invalid " + key + " value: " + val);
        return false;
    }
    
    target = invert ? (val == "off") : (val == "on");
    return true;
}

bool RealTimeTune::ParseTimerConfig(const YAML::Node& config)
{
    if (!config["timer"] || !config["timer"]["migration"]) {
        ERROR(logger, "timer missing or not map");
        return false;
    }
    
    auto migNode = config["timer"]["migration"];
    if (!migNode.IsScalar()) {
        ERROR(logger, "timer.migration must be scalar (on/off)");
        return false;
    }
    
    std::string mig = migNode.as<std::string>();
    if (!IsValidSwitch(mig)) {
        ERROR(logger, "Invalid timer.migration value: " + mig);
        return false;
    }
    
    switchConfig.timerMigration = (mig == "off");
    return true;
}

bool RealTimeTune::ParseSchedConfig(const YAML::Node& config)
{
    if (!config["sched"] || !config["sched"]["rt_runtime_us"]) {
        ERROR(logger, "sched missing or not map");
        return false;
    }
    
    auto rtNode = config["sched"]["rt_runtime_us"];
    if (!rtNode.IsScalar()) {
        ERROR(logger, "sched.rt_runtime_us must be scalar (on/off)");
        return false;
    }
    
    std::string rtStr = rtNode.as<std::string>();
    if (!IsValidSwitch(rtStr)) {
        ERROR(logger, "Invalid sched.rt_runtime_us value, only 'off' or 'on' allowed");
        return false;
    }
    
    switchConfig.rtRuntimeUs = (rtStr == "off") ? -1 : 0;
    return true;
}

RealtimeTuneSwitch RealTimeTune::GetSwitchConfig()
{
    return switchConfig;
}

// 解析range字符串，返回未包含在range内的CPU编号字符串（逗号分隔）
std::string RealTimeTune::GetNotRangeCpus(const std::string& range, int maxCpu)
{
    std::vector<int> includeCpus = oeaware::ParseRange(range);
    
    // 将 includeCpus 转换为 set 以便快速查找
    std::set<int> includeSet(includeCpus.begin(), includeCpus.end());
    
    // 找出不在范围内的 CPU
    std::vector<int> notInRange;
    for (int i = 0; i <= maxCpu; ++i) {
        if (includeSet.find(i) == includeSet.end()) {
            notInRange.push_back(i);
        }
    }
    
    // 合并连续区间
    std::ostringstream out;
    size_t n = notInRange.size();
    for (size_t i = 0; i < n; ) {
        int start = notInRange[i];
        int end = start;
        while (i + 1 < n && notInRange[i + 1] == end + 1) {
            end = notInRange[++i];
        }
        if (!out.str().empty()) out << ",";
        if (start == end) {
            out << start;
        } else {
            out << start << "-" << end;
        }
        ++i;
    }
    return out.str();
}

bool RealTimeTune::IsGrubbyAvailable()
{
    int ret = system("grubby --version > /dev/null 2>&1");
    return ret == 0;
}

std::string RealTimeTune::GetGrubbyInfo()
{
    std::string result;
    if (!oeaware::ExecCommand("grubby --info=DEFAULT", result)) {
        INFO(logger, "Failed to run grubby --info=DEFAULT");
        return "";
    }
    return result;
}

std::map<std::string, std::string> RealTimeTune::ParseGrubbyCmdline(const std::string& grubbyInfo)
{
    std::map<std::string, std::string> paramMap;
    std::istringstream iss(grubbyInfo);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("args=") == 0) {
            size_t firstQuote = line.find('"');
            size_t lastQuote = line.rfind('"');
            if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote) {
                std::string args = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                std::istringstream argsStream(args);
                std::string token;
                while (argsStream >> token) {
                    size_t eq = token.find('=');
                    if (eq != std::string::npos) {
                        std::string key = token.substr(0, eq);
                        std::string value = token.substr(eq + 1);
                        paramMap[key] = value;
                    } else {
                        paramMap[token] = "";
                    }
                }
            }
            break;
        }
    }
    return paramMap;
}

bool RealTimeTune::CheckAndUpdateKernelParamsWithGrubby()
{
    if (!IsGrubbyAvailable()) {
        INFO(logger, "grubby is not available on this system, please install grubby first.");
        return false;
    }
    
    std::string grubbyInfo = GetGrubbyInfo();
    if (grubbyInfo.empty()) {
        INFO(logger, "Failed to get grubby info, skip kernel param check.");
        return false;
    }
    
    std::map<std::string, std::string> expectedParams = GenerateExpectedKernelParams(maxCpuId);
    auto currentParams = ParseGrubbyCmdline(grubbyInfo);
    
    // 记录需要更新的参数，用于回退
    std::vector<std::pair<std::string, std::string>> updatedParams;
    std::vector<std::string> failedUpdates;
    
    // 检查并更新参数
    for (const auto& kv : expectedParams) {
        auto it = currentParams.find(kv.first);
        if (it == currentParams.end() || it->second != kv.second) {
            INFO(logger, "Kernel param " + kv.first + " is not as expected. Current: " +
                (it == currentParams.end() ? "not set" : it->second) +
                ", Expected: " + kv.second + ". Will update.");
            
            // 记录当前值用于回退
            std::string currentValue = (it == currentParams.end() ? "" : it->second);
            updatedParams.push_back({kv.first, currentValue});
            
            // 尝试更新参数
            if (!UpdateKernelParamWithGrubby(kv.first, kv.second)) {
                ERROR(logger, "Failed to update kernel param: " + kv.first);
                failedUpdates.push_back(kv.first);
            }
        } else {
            INFO(logger, "Kernel param " + kv.first + " is as expected: " + kv.second);
        }
    }
    
    // 如果有更新失败，进行回退
    if (!failedUpdates.empty()) {
        ERROR(logger, "Some kernel params failed to update, rolling back changes.");
        
        // 回退所有已更新的参数
        for (const auto& param : updatedParams) {
            if (param.second.empty()) {
                // 原值为空，需要移除参数
                if (!RemoveKernelParamWithGrubby(param.first)) {
                    ERROR(logger, "Failed to remove kernel param during rollback: " + param.first);
                }
            } else {
                // 恢复原值
                if (!UpdateKernelParamWithGrubby(param.first, param.second)) {
                    ERROR(logger, "Failed to restore kernel param during rollback: " + param.first);
                }
            }
        }
        
        return false;
    }
    
    // 如果有参数被更新，需要更新 grub 配置
    if (!updatedParams.empty()) {
        if (!isRebooting) {
            isRebooting = true; // 重启流程中设为true
        }
        
        int ret = system("grub2-mkconfig -o /boot/grub2/grub.cfg");
        if (ret != 0) {
            ERROR(logger, "Failed to update grub config, rolling back kernel param changes.");
            
            // grub 配置更新失败，回退内核参数
            for (const auto& param : updatedParams) {
                if (param.second.empty()) {
                    if (!RemoveKernelParamWithGrubby(param.first)) {
                        ERROR(logger, "Failed to remove kernel param during grub rollback: " + param.first);
                    }
                } else {
                    if (!UpdateKernelParamWithGrubby(param.first, param.second)) {
                        ERROR(logger, "Failed to restore kernel param during grub rollback: " + param.first);
                    }
                }
            }
            
            return false;
        } else {
            INFO(logger, "Grub config updated successfully.");
        }
    }
    
    return true;
}

std::map<std::string, std::string> RealTimeTune::GenerateExpectedKernelParams(int maxCpu)
{
    std::map<std::string, std::string> params;
    for (const auto& key : KERNEL_PARAM_KEYS) {
        auto it = switchConfig.featureSwitches.find(key);
        if (it != switchConfig.featureSwitches.end() && it->second) {
            if (key == "irqaffinity") {
                // 使用缓存的 maxCpuId
                std::vector<int> includeCpus = oeaware::ParseRange(switchConfig.cpuIsolationRange);
                std::string notRange = GetNotRangeCpus(switchConfig.cpuIsolationRange, maxCpu);
                params[key] = notRange;
            } else {
                params[key] = switchConfig.cpuIsolationRange;
            }
        }
    }
    return params;
}

bool RealTimeTune::UpdateKernelParamWithGrubby(const std::string& key, const std::string& value) const
{
    // 拼接参数字符串
    std::ostringstream oss;
    oss << "grubby --update-kernel=/boot/vmlinuz-$(uname -r) --args=\"" << key << "=" << value << "\"";
    std::string cmd = oss.str();
    INFO(logger, "Run: " + cmd);
    int ret = system(cmd.c_str());
    if (ret != 0) {
        INFO(logger, "Failed to update kernel param " + key + " with grubby.");
        return false;
    }
    return true;
}

void RealTimeTune::DisableKernelParams()
{
    for (const auto& key : KERNEL_PARAM_KEYS) {
        auto it = switchConfig.featureSwitches.find(key);
        if (it != switchConfig.featureSwitches.end() && it->second) {
            // 只有开关为 on 的参数才移除
            RemoveKernelParamWithGrubby(key);
        } else {
            INFO(logger, "Kernel param " + key + " is not enabled, skip removing.");
        }
    }
    INFO(logger, "Kernel params removed, please reboot for changes to take effect.");
}

bool RealTimeTune::RemoveKernelParamWithGrubby(const std::string& key) const
{
    std::ostringstream oss;
    oss << "grubby --update-kernel=/boot/vmlinuz-$(uname -r) --remove-args=\"" << key << "\"";
    std::string cmd = oss.str();
    INFO(logger, "Run: " + cmd);
    int ret = system(cmd.c_str());
    if (ret != 0) {
        INFO(logger, "Failed to remove kernel param " + key + " with grubby.");
        return false;
    }
    return true;
}
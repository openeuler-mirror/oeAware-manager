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
#ifndef REALTIME_TUNE_H
#define REALTIME_TUNE_H
#include "oeaware/interface.h"
#include <yaml-cpp/yaml.h>

struct RealtimeTuneSwitch
{
	bool cpufreqPerformance = false;
	bool transparentHugepage = false;
	bool numaBalancing = false;
	bool ksm = false;
	bool swap = false;
	bool timerMigration = false;
	int rtRuntimeUs = 0;
	std::map<std::string, bool> featureSwitches; // 保存cpu_isolation.features的开关状态
	std::string cpuIsolationRange; // 保存cpu_isolation.range的值
};

class RealTimeTune : public oeaware::Interface {
public:
	RealTimeTune();
	~RealTimeTune() override = default;
	oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
	void CloseTopic(const oeaware::Topic &topic) override;
	void UpdateData(const DataList &dataList) override;
	oeaware::Result Enable(const std::string &param) override;
	void Disable() override;
	void Run() override;

private:
    bool CheckEnv();
	void UpdateMaxCpuId();
	int GetMaxCpuId();
	bool ParseFromSwitchConfig(const std::string& configPath = oeaware::DEFAULT_REALTIMETUNE_CONFIG_PATH);
	std::string GetNotRangeCpus(const std::string& range, int maxCpu);
	bool AddRealtimeTuneToConfig(const std::string& configPath = oeaware::DEFAULT_CONFIG_PATH);
	bool RemoveRealtimeTuneFromConfig(const std::string& configPath = oeaware::DEFAULT_CONFIG_PATH);
	
	bool IsGrubbyAvailable();  // 检查grubby是否可用
	std::string GetGrubbyInfo();  // 获取grubby信息
	std::map<std::string, std::string> ParseGrubbyCmdline(const std::string& grubbyInfo);  // 解析grubby信息
    std::map<std::string, std::string> GenerateExpectedKernelParams(int maxCpu);  // 生成预期参数
	bool CheckAndUpdateKernelParamsWithGrubby();  // 检查并更新grubby参数
	bool UpdateKernelParamWithGrubby(const std::string& key, const std::string& value) const;  // 更新grubby参数
	void DisableKernelParams();  // 禁用grubby参数
	bool RemoveKernelParamWithGrubby(const std::string& key) const;  // 移除grubby参数

	void DisableProcAndSys();
	bool OpenProcAndSys();
	bool SetStringValue(const std::string& path, const std::string& value);
	bool SetCpuFreqPerformance();
	bool GetInitialValue(const std::string& path, long& value);
	// 测试用的公共接口
	void SetTestConfig(const std::string& range, const std::map<std::string, bool>& switches) {
		switchConfig.cpuIsolationRange = range;
		switchConfig.featureSwitches = switches;
	}
	RealtimeTuneSwitch GetSwitchConfig();

	const std::string timerMigrationPath = "/proc/sys/kernel/timer_migration";
	const std::string schedRuntimePath = "/proc/sys/kernel/sched_rt_runtime_us";
	const std::string ksmRunPath = "/sys/kernel/mm/ksm/run";
	const std::string thpEnabledPath = "/sys/kernel/mm/transparent_hugepage/enabled";
	const std::string numaBalancingPath = "/proc/sys/kernel/numa_balancing";
	const std::string swapoffCmd = "swapoff -a";
	const std::string swaponCmd = "swapon -a";
	const std::string cpuFreqPath = "/sys/devices/system/cpu/";
	const int defaultPeriod = 1000;
	const int defaultPriority = 2;
	bool stateRestored = false;
	struct SysTuneBackup {
		long ksmRun = -1;
		std::string thpEnabled;
		long numaBalancing = -1;
		long timerMigration = -1;
		long schedRtRuntime = -1;
		std::unordered_map<std::string, std::string> cpuGovs; // 保存每个cpu的governor策略
	} backup;
	RealtimeTuneSwitch switchConfig;
	std::string GetCurrentThpMode(); // 新增
	bool isRebooting = false; // 默认为false，重启流程中设为true
	const std::vector<std::string> KERNEL_PARAM_KEYS = {"isolcpus", "nohz_full", "rcu_nocbs", "irqaffinity"}; // 内核参数键列表
	int maxCpuId = 0; // 在线CPU数量
	
	// 配置解析相关的私有函数
	bool ParseCpuIsolationConfig(const YAML::Node& config);
	bool ParseCpuIsolationRange(const YAML::Node& cpuIso);
	bool ValidateCpuRange(const std::string& rangeValue);
	bool ValidateCpuToken(const std::string& token);
	bool ParseCpuIsolationFeatures(const YAML::Node& cpuIso);
	bool ParseCpuFreqConfig(const YAML::Node& config);
	bool ParseMemoryConfig(const YAML::Node& config);
	bool ParseMemorySwitch(const YAML::Node& mem, const std::string& key, bool& target, bool invert);
	bool ParseTimerConfig(const YAML::Node& config);
	bool ParseSchedConfig(const YAML::Node& config);
};
#endif
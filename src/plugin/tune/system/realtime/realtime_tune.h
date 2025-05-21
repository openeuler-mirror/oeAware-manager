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
	bool SetValue(const std::string& path, long value);
	bool GetInitialValue(const std::string& path, long& value);
	const std::string timerMigrationPath = "/proc/sys/kernel/timer_migration";
	const std::string schedRuntimePath = "/proc/sys/kernel/sched_rt_runtime_us";
	long defaultTimerMigration = 0;
	long defaultSchedRuntime = 0;
	const int defaultPeriod = 1000;
	const int defaultPriority = 2;
	bool stateRestored = false;
};
#endif
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
#include <iostream>
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
    bool setDefaultTimerMigration = false;
    bool setDefaultSchedRuntime = false;
    bool getDefaultTimerMigration = GetInitialValue(timerMigrationPath, defaultTimerMigration);
	bool getDefaultSchedRuntime = GetInitialValue(schedRuntimePath, defaultSchedRuntime);
    stateRestored = false;
	if (getDefaultTimerMigration) {
        INFO(logger, "[Realtime]Initial DefaultTimerMigration: " + std::to_string(defaultTimerMigration));
        setDefaultTimerMigration = SetValue(timerMigrationPath, 0);
	} else {
        ERROR(logger, "[Realtime]Fail to get DefaultTimerMigration.");
    }

    if (getDefaultSchedRuntime) {
        INFO(logger, "[Realtime]Initial defaultSchedRuntime: " + std::to_string(defaultSchedRuntime));
        setDefaultSchedRuntime = SetValue(schedRuntimePath, -1);
    } else {
        ERROR(logger, "[Realtime]Fail to get defaultSchedRuntime.");
    }
   
    if (setDefaultTimerMigration && setDefaultSchedRuntime) {
        return oeaware::Result(OK, "Realtime enabled successfully");
	} else {
        return oeaware::Result(FAILED, "Failed to enable Realtime");
    }
}

void RealTimeTune::Disable()
{
    bool setDefaultTimerMigration = false;
    bool setDefaultSchedRuntime = false;
    if (!stateRestored) {
        setDefaultTimerMigration = SetValue(timerMigrationPath, defaultTimerMigration);
        setDefaultSchedRuntime = SetValue(schedRuntimePath, defaultSchedRuntime);
	    if (setDefaultTimerMigration && setDefaultSchedRuntime) {
		    stateRestored = true;
		    INFO(logger, "[Realtime]Set default successfully.");
	    } else {
		    ERROR(logger, "[Realtime]Failed to restore.");
	    }
    }
}

void RealTimeTune::Run()
{
}

bool RealTimeTune::GetInitialValue(const std::string& path, long& value)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        ERROR(logger, "[RealTimeTune] Failed to open file: " + path);
        return false;
    }

    std::string line;
    if (!std::getline(file, line)) {
        ERROR(logger, "[RealTimeTune] Failed to read from file: " + path);
        file.close();
        return false;
    }
    file.close();

    std::istringstream iss(line);
    int parsedValue;
    if (!(iss >> parsedValue)) {
        ERROR(logger, "[RealTimeTune] Failed to parse value from file: " + path);
        return false;
    }
    value = parsedValue;
    return true;
}

bool RealTimeTune::SetValue(const std::string& path, long value)
{
        std::ofstream file(path);
        if (!file.is_open()) {
            ERROR(logger, "[RealTimeTune] Failed to open file for writing: " + path);
            return false;
        }
        file << value;
        if (!file.good()) {
            ERROR(logger, "[RealTimeTune] Failed to write to file: " + path);
            file.close();
            return false;
        }
        file.flush();
        file.close();
        return true;
}

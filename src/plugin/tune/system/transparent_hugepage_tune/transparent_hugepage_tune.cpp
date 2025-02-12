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
#include "transparent_hugepage_tune.h"

using namespace oeaware;
TransparentHugepageTune::TransparentHugepageTune():thpStateRestored(false)
{
	name = OE_TRANSPARENT_HUGEPAGE_TUNE;
	description = "To enable and utilize the THP feature";
	version = "1.0.0";
	period = defaultPeriod; // 1000 period is meaningless, only Enable() is executed, not Run()
	priority = defaultPriority;
	type = oeaware::TUNE;
}
oeaware::Result TransparentHugepageTune::OpenTopic(const oeaware::Topic &topic)
{
	(void)topic;
	return oeaware::Result(OK);
}

void TransparentHugepageTune::CloseTopic(const oeaware::Topic &topic)
{
	(void)topic;
}

void TransparentHugepageTune::UpdateData(const DataList &dataList)
{
	(void)dataList;
}

oeaware::Result TransparentHugepageTune::Enable(const std::string &param)
{
    (void)param;
    initialThpMode = GetInitialThpMode();
	INFO(logger, "[THP]Initial THP mode: " + initialThpMode);
    thpStateRestored = false;
    if (SetThpMode("always") == 0) {
        return oeaware::Result(OK, "THP enabled successfully");
	} else {
        return oeaware::Result(FAILED, "Failed to enable THP");
    }
}

void TransparentHugepageTune::Disable()
{
	INFO(logger, "[THP]Restoring THP mode to: " + initialThpMode);
	if (!thpStateRestored) {
		if (SetThpMode(initialThpMode) == 0) {
			thpStateRestored = true;
			INFO(logger, "[THP]Mode set sucessfully.");
		} else {
			ERROR(logger, "[THP]Failed to restore THP mode.");
		}
	}
}

void TransparentHugepageTune::Run()
{
}

std::string TransparentHugepageTune::GetInitialThpMode()
{
	std::ifstream thpFile(thpFileName);
	if (thpFile.is_open()) {
		std::string modes;
		std::getline(thpFile, modes);
		thpFile.close();
		size_t startPos = modes.find('[') + 1;
		size_t endPos = modes.find(']', startPos);
		if (startPos != std::string::npos && endPos != std::string::npos) {
			std::string currentMode = modes.substr(startPos, endPos - startPos);
			return currentMode;
		}
	}
	ERROR(logger, "[THP]Failed to read initial THP mode.");
	return "";
}

int TransparentHugepageTune::SetThpMode(const std::string &mode)
{
	if (mode.empty()) {
		ERROR(logger, "[THP]Empty mode string provided.");
		return -1;
	}
	std::ofstream thpFile(thpFileName);
	if (!thpFile.is_open()) {
		ERROR(logger, "[THP]Failed to open THP mode file for writing.");
		return -1;
	}
	thpFile << mode;
	thpFile.close();
	INFO(logger, "[THP] Mode set to: " + mode);
	return 0;
}

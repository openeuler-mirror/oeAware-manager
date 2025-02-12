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
#ifndef TRANSPARENT_HUGEPAGE_TUNE_H
#define TRANSPARENT_HUGEPAGE_TUNE_H
#include "oeaware/interface.h"

class TransparentHugepageTune : public oeaware::Interface {
public:
	TransparentHugepageTune();
	~TransparentHugepageTune() override = default;
	oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
	void CloseTopic(const oeaware::Topic &topic) override;
	void UpdateData(const DataList &dataList) override;
	oeaware::Result Enable(const std::string &param) override;
	void Disable() override;
	void Run() override;
private:
	int SetThpMode(const std::string &mode);
	std::string GetInitialThpMode();
	const std::string thpFileName = "/sys/kernel/mm/transparent_hugepage/enabled";
	const int defaultPeriod = 1000;
	const int defaultPriority = 2;
	std::string initialThpMode;
	bool thpStateRestored = false;
};
#endif
/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef SMC_D_SCENARIO_AWARE_H
#define SMC_D_SCENARIO_AWARE_H
#include "oeaware/interface.h"
#include "analysis.h"

namespace oeaware {
class SmcDAnalysis : public Interface {
public:
    SmcDAnalysis();
    ~SmcDAnalysis() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
	void CloseTopic(const oeaware::Topic &topic) override;
	void UpdateData(const DataList &dataList) override;
	Result Enable(const std::string &param) override;
	void Disable() override;
	void Run() override;

private:
    std::string ExecuteCommand(const std::string &cmd);
    int GetConnectionCount(const std::string &state);
    void DetectTcpConnectionMode();
    void* GetResult();
    void PublishData();
    
private:
    std::vector<std::string> topicStrs{"smc_d"};
    int analysisTime = 10;
    int curTime = 0;
    bool isPublished = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> beginTime;
    Topic saveTopic;
    int prevEstablishedCount = 0; // 上一次的 ESTABLISHED 连接数
    int prevCloseWaitCount = 0; // 上一次的 CLOSE_WAIT 连接数
    int totalEstablishedDelta = 0; // 累计 ESTABLISHED 变化量
    int totalCloseWaitDelta = 0; // 累计 CLOSE_WAIT 变化量
};
}
#endif
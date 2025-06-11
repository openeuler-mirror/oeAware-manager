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
#ifndef XCALL_ANALYSIS_H
#define XCALL_ANALYSIS_H
#include <unordered_set>
#include <oeaware/interface.h>
#include <oeaware/data/analysis_data.h>

namespace oeaware {
struct XcallInfo {
    int calls;
    double time;
    std::string callName;
};
class XcallAnalysis : public Interface {
public:
    XcallAnalysis();
    ~XcallAnalysis() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
    void SetTopicResult(const std::string &type, const std::string &res);
private:
    void InitSyscallTable();
    void PublishData(const Topic &topic);
    struct XcallTopic {
        bool open;
        bool isPublish;
        int pid;
        int time; // analysis time
        int cnt = 0;
        double cpuSys = 0;
        std::string pName;
        std::chrono::time_point<std::chrono::steady_clock> beginTime;
    };
    void Analysis(const XcallTopic &xcallTopic);
    void OutXcallConfig(int pid, const std::string &pName);
    void CreateXcallYaml(int pid, const std::string &pName, const std::vector<XcallInfo> &vec);
    std::vector<std::string> topicStrs{"xcall"};
    std::unordered_map<std::string, XcallTopic> topicStatus;
    std::unordered_map<std::string, int> syscallTable;
    std::vector<Topic> subscribeTopics;
    AnalysisResultItem analysisResultItem = {};
    int curTime = 0;
    double threshold = 5;
    double cpuSystemUsage = 0;
    int topNum = 5;
    const std::unordered_set<std::string> pretetchSyscalls{"epoll_pwait"};
};
}
#endif


/******************************************************************************
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef XCALL_TUNE_H
#define XCALL_TUNE_H
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include "oeaware/interface.h"
const std::vector<std::vector<int>> XCALL_RANGE = {
    {0, 294},
    {403, 469},
};
class XcallTune : public oeaware::Interface {
public:
    XcallTune();
    ~XcallTune() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
private:
    int ReadConfig(const std::string &path);
    int WriteSysParam(const std::string &path, const std::string &value);
    void EnablePrefetchCpu();
    const int defaultPeriod = 1000;
    const int defaultPriority = 2;
    std::string configPath;
    std::unordered_set<std::string> xcallType{"xcall_1", "xcall_2"};
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> xcallTune;
    std::unordered_map<std::string, std::unordered_set<int>> threadId;
    std::unordered_map<std::string, std::vector<std::string>> openedXcall;
};

#endif

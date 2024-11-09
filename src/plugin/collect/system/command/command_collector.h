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

#ifndef COMMAND_COLLECTOR_H
#define COMMAND_COLLECTOR_H

#include "command_base.h"
#include "interface.h"

class CommandCollector : public oeaware::Interface {
public:
    CommandCollector();
    ~CommandCollector() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &parma = "") override;
    void Disable() override;
    void Run() override;
private:
    std::vector<std::string> topicStr = {"mpstat", "iostat", "vmstat", "sar", "pidstat", "lscpu", "zone_reclaim_mode",
                                         "meminfo", "ethtool", "ifconfig", "os-release", "version"};
    std::unordered_map<std::string, std::unique_ptr<CommandBase>> collectors;
    std::unordered_map<std::string, std::thread> collectThreads;
    std::unordered_map<std::string, std::thread> publishThreads;
    std::unordered_map<std::string, std::mutex> topicMutex;
    void CollectThread(const oeaware::Topic &topic, CommandBase* collector);
    void PublishThread(const oeaware::Topic &topic, CommandBase* collector);
};

#endif

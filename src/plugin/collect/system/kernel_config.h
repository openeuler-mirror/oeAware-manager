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
#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H
#include <unordered_map>
#include <unordered_set>
#include <string>
#include "data_list.h"
#include "interface.h"
#include "kernel_data.h"
/*
 * topic: get_kernel_config, obtain the kernel parameter information.
 * params: kernel params name, including
 * 1. sysctl -a -N
 *
 * topic: get_cmd, the trustlist command is supported.
 * params: each command is seqarated by "@@", include "cat", "grep", "awk", "pgrep", "ls", "ethtool".
 *
 * topic: set_kernel_config, modify kernel parameters.
 * DataList:
 *  data: KernelData, [key, value]:
 *  1. [path, value], like [/sys/block/sda/queue/max_sectors_kb, 512].
 *  2. [cmd, _], run the shell command to modify parameters.
*/
class KernelConfig : public oeaware::Interface {
public:
    KernelConfig();
    ~KernelConfig() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param = "") override;
    void Disable() override;
    void Run() override;
private:
    void PublishCmd();
    void PublishKernelParams();
    void PublishKernelConfig();
    void SetKernelConfig();
    bool InitCmd(std::stringstream &ss, const std::string &topicType);
    void InitFileParam();
    void AddCommandParam(const std::string &cmd);
    void WriteSysParam(const std::string &path, const std::string &value);
    void GetAllEth();

    std::vector<std::string> topicStr = {"get_kernel_config", "get_cmd", "set_kernel_config"};
    // key: topic type, value: parameters to be queried.
    std::unordered_map<std::string, std::unordered_set<std::string>> getTopics;
    std::unordered_map<std::string, std::vector<std::string>> getCmds;
    std::vector<std::pair<std::string, std::string>> setSystemParams;
    
    std::unordered_map<std::string, std::string> sysctlParams;
    std::vector<std::string> cmdRun;
    static std::vector<std::string> getCmdGroup;
    static std::vector<std::string> cmdGroup;
    const std::string cmdSeparator = "@@";
};

#endif

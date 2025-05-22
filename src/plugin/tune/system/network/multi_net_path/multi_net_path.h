/******************************************************************************
 * Copyright (c) 2025-2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef MULTI_NET_PATH_TUNE
#define MULTI_NET_PATH_TUNE

#include "oeaware/interface.h"

namespace oeaware {
class MultiNetPath : public Interface {
public:
    MultiNetPath();
    ~MultiNetPath() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
private:
    const int invaildParam = -1;  // -1 means not read from params or invaild
    bool irqbalanceStatus = false; // irqbalance status before tune
    std::string oenetcls = "oenetcls";
    std::string cmd;
    std::string ifnamePara;
    std::vector<std::string> ifname;  // split ifnamePara by #
    std::string appname = "redis-server";
    int matchIp = invaildParam; // ko inner default is 1
    int mode = invaildParam;
    std::unordered_map<std::string, std::string> cmdHelp = {
        {"ifname", "Configure multiple network interfaces, separated by # "},
        {"appname", "Configure the name of the network process, default is redis-server, all_app means all processes"},
        {"match_ip", "Configure whether to match the IP address, 0: not match, 1: match(default)"},
        {"mode", "Configure the mode of the network process, can config 0(defaule) and 1"}
    };
    bool CheckLegal(const std::string &cmd);
    bool ResolveCmd(const std::string &cmd);
    bool CheckParam();
    void ShowHelp();
    bool InsertOeNetCls();
};

}

#endif // !MULTI_NET_PATH_TUNE

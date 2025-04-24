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

#ifndef OEAWARE_NET_INTERFACE_H
#define OEAWARE_NET_INTERFACE_H
#include <unordered_map>
#include <unordered_set>
#include <string>
#include "net_intf_comm.h"
#include "oeaware/interface.h"
#include "oeaware/data/network_interface_data.h"

class NetInterface : public oeaware::Interface {
public:
    NetInterface();
    ~NetInterface() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param = "") override;
    void Disable() override;
    void Run() override;
private:
    struct NetIntTopic {
        std::string topicName;
        std::vector<std::string> supportParams;
        std::unordered_set<std::string> openedParams;
    };
    std::vector<std::string> topicStr = { OE_NETWORK_INTERFACE_BASE_TOPIC, OE_NETWORK_INTERFACE_DRIVER_TOPIC };
    std::unordered_map<std::string, NetIntfBaseInfo> netIntfBaseInfo; // intf to share info
    std::unordered_map<std::string, NetIntTopic> netTopicInfo; // topic name to topic info
    void InitTopicInfo(const std::string &name);
    void PublishBaseInfo(const std::string &params);
    void PublishDriverInfo(const std::string &params);
    oeaware::Result OpenNetFlow();
    void CloseNetFlow();
    void ReadFlow();

    void *skel = nullptr;
};

#endif // OEAWARE_NET_INTERFACE_H
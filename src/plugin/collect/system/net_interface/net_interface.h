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
#include <functional>
#include "net_intf_comm.h"
#include "oeaware/interface.h"
#include "oeaware/data/network_interface_data.h"

struct SockKey {
    uint32_t localIp;
    uint32_t remoteIp;
    uint16_t localPort;
    uint16_t remotePort;
    bool operator==(const SockKey &other) const
    {
        return std::tie(localIp, remoteIp, localPort, remotePort) ==
            std::tie(other.localIp, other.remoteIp, other.localPort, other.remotePort);
    }
};
namespace std {
    template<>
    struct hash<SockKey> {
        size_t operator()(const SockKey& key) const {
            size_t h1 = std::hash<uint32_t>()(key.localIp);
            size_t h2 = std::hash<uint32_t>()(key.remoteIp);
            size_t h3 = std::hash<uint16_t>()(key.localPort);
            size_t h4 = std::hash<uint16_t>()(key.remotePort);
            // 1 2 3 : simple method to combine the hashes
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };
}

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
    std::vector<std::string> topicStr = { OE_NETWORK_INTERFACE_BASE_TOPIC,
        OE_NETWORK_INTERFACE_DRIVER_TOPIC, OE_LOCAL_NET_AFFINITY };
    std::unordered_map<std::string, NetIntfBaseInfo> netIntfBaseInfo; // intf to share info
    std::unordered_map<std::string, NetIntTopic> netTopicInfo; // topic name to topic info
    std::unordered_map<SockKey, uint64_t> lastSockFlow;
    void InitTopicInfo(const std::string &name);
    void PublishBaseInfo(const std::string &params);
    void PublishDriverInfo(const std::string &params);
    void PublishLocalNetAffiInfo(const std::string &params);
    bool OpenNetFlow();
    void CloseNetFlow();
    void ReadFlow(std::unordered_map<uint64_t, uint64_t> &flowData);

    void *skel = nullptr;
};

#endif // OEAWARE_NET_INTERFACE_H
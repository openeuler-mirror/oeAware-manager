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
#include <chrono>
#include <bpf/libbpf.h>
#include <bpf/libbpf_common.h>
#include "net_intf_comm.h"
#include "ebpf/net_flow_comm.h"
#include "oeaware/interface.h"
#include "oeaware/data/network_interface_data.h"

inline bool operator==(const SockKey &lhs, const SockKey &rhs)
{
    return std::tie(lhs.localIp, lhs.localPort, lhs.remoteIp, lhs.remotePort) ==
           std::tie(rhs.localIp, rhs.localPort, rhs.remoteIp, rhs.remotePort);
}

namespace std {
    template<>
    struct hash<SockKey> {
        size_t operator()(const SockKey& key) const {
            size_t h1 = std::hash<uint32_t>()(key.localIp);
            size_t h2 = std::hash<uint32_t>()(key.remoteIp);
            size_t h3 = std::hash<uint16_t>()(key.localPort);
            size_t h4 = std::hash<uint16_t>()(key.remotePort);
            // 1 2 3 : simple method to combine the hashes (FNV-1a)
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
    struct NetDevHook {
        bpf_tc_hook tcHook;
        bpf_tc_opts tcOpts;
        int ifindex;
    };
private:
    struct NetIntTopic {
        std::string topicName;
        std::vector<std::string> supportParams;
        std::unordered_set<std::string> openedParams;
        std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
    };
    std::vector<std::string> topicStr = { OE_NETWORK_INTERFACE_BASE_TOPIC,
        OE_NETWORK_INTERFACE_DRIVER_TOPIC, OE_LOCAL_NET_AFFINITY, OE_NET_THREAD_QUE_DATA };
    std::unordered_map<int, NetIntfBaseInfo> netIntfBaseInfo; // key is ifindex
    std::unordered_map<std::string, NetIntTopic> netTopicInfo; // topic name to topic info
    std::unordered_map<std::string, bool> debugCtl;
    void InitTopicInfo(const std::string &name);
    void PublishBaseInfo(const std::string &params);
    void PublishDriverInfo(const std::string &params);
    void PublishLocalNetAffiInfo(const std::string &params);
    void PublishNetQueueInfo(const std::string &params, const int &interval);
    bool AttachTcProgram(struct net_flow_kernel *obj, std::string name, int ifindex);
    bool OpenNetFlow(const std::string &topicName);
    void CloseNetFlow(const std::string &topicName);
    void ReadFlow(std::unordered_map<uint64_t, uint64_t> &flowData);
    void ReadNetQueue(std::vector<QueueInfo> &threadQueData);

    struct NetFlowCtl {
        // ebpf net comm
        void *skel = nullptr;
        std::unordered_map<int, NetDevHook> netDevHooks; // key is ifindex
        std::unordered_set<std::string> openTopic;
        // local net process info
        std::unordered_map<SockKey, uint64_t> lastSockFlow;
        // remote recv queue info
        std::unordered_map<SockKey, uint64_t> lastQueTimes;
        std::unordered_map<SockKey, uint64_t> lastQueLen;

    } netFlowCtl;
};

#endif // OEAWARE_NET_INTERFACE_H
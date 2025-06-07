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
#include "net_interface.h"
#include <iostream>
#include <cstdint>
#include <arpa/inet.h>
#include <bpf/libbpf.h>
#include <bpf/libbpf_common.h>
#include <net/if.h>
#include <sys/resource.h>
#include "ebpf/net_flow.skel.h"

constexpr int UINT32_BIT_LEN = 32;
constexpr uint64_t UINT32_MASK = 0xFFFFFFFFULL;
constexpr uint64_t TIME_STR_LEN = 20;
// Used to uniquely determine the packet receiving information of an interrupt on a thread
struct ThreadQueKey {
    uint32_t tid;
    int ifindex;
    int queueId;
    bool operator==(const ThreadQueKey &other) const
    {
        return std::tie(tid, ifindex, queueId) == std::tie(other.tid, other.ifindex, other.queueId);
    }
};

namespace std {
    template<>
    struct hash<ThreadQueKey> {
        size_t operator()(const ThreadQueKey &key) const
        {
            size_t h1 = std::hash<uint32_t>()(key.tid);
            size_t h2 = std::hash<uint32_t>()(key.ifindex);
            size_t h3 = std::hash<uint16_t>()(key.queueId);
            // 1 2 simple method to combine the hashes (FNV-1a)
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

/* use this function to filter publish data */
bool IfTopicParamsData(const std::string &params, const NetIntfBaseInfo &info)
{
    if (params == "operstate_up") {
        return info.operstate == IF_OPER_UP;
    } else if (params == "operstate_all") {
        return true;
    }
    return false;
}

NetInterface::NetInterface()
{
    name = OE_NET_INTF_INFO;
    version = "1.0.0";
    period = 1000; // 1000ms, consider reducing period in the future
    priority = 0;
    type = 0;
    for (const auto &it : topicStr) {
        oeaware::Topic topic;
        topic.instanceName = this->name;
        topic.topicName = it;
        topic.params = "";
        supportTopics.push_back(topic);
        InitTopicInfo(it);
    }
}

oeaware::Result NetInterface::OpenTopic(const oeaware::Topic &topic)
{
    if (netTopicInfo.count(topic.topicName) == 0) {
        return oeaware::Result(FAILED, "topic not register");
    }
    const std::vector<std::string> &supportParams = netTopicInfo[topic.topicName].supportParams;
    if (find(supportParams.begin(), supportParams.end(), topic.params) == supportParams.end()) {
        return oeaware::Result(FAILED, "topic not support params");
    }
    if (netTopicInfo[topic.topicName].openedParams.count(topic.params) != 0) {
        return oeaware::Result(FAILED, "topic already opened");
    }

    if (topic.topicName == OE_LOCAL_NET_AFFINITY) {
        if (topic.params == OE_PARA_PROCESS_AFFINITY && !OpenNetFlow(topic.topicName)) {
            return oeaware::Result(FAILED, "open net flow failed");
        } else if (topic.params == OE_PARA_LOC_NET_AFFI_USER_DEBUG || topic.params == OE_PARA_LOC_NET_AFFI_KERN_DEBUG) {
            debugCtl[topic.params] = true;
        }
    }

    if (topic.topicName == OE_NET_THREAD_QUE_DATA) {
        if (topic.params == OE_PARA_THREAD_RECV_QUE_CNT && !OpenNetFlow(topic.topicName)) {
            return oeaware::Result(FAILED, "open net flow failed");
        } else if (topic.params == OE_PARA_NET_RECV_QUE_USER_DEBUG || topic.params == OE_PARA_NET_RECV_QUE_KERN_DEBUG) {
            debugCtl[topic.params] = true;
        }
    }

    netTopicInfo[topic.topicName].openedParams.insert(topic.params);
    return oeaware::Result(OK);
}

void NetInterface::CloseTopic(const oeaware::Topic &topic)
{
    if (netTopicInfo.count(topic.topicName) == 0) {
        return;
    }
    const std::vector<std::string> &supportParams = netTopicInfo[topic.topicName].supportParams;
    if (find(supportParams.begin(), supportParams.end(), topic.params) == supportParams.end()) {
        return;
    }
    if (netTopicInfo[topic.topicName].openedParams.count(topic.params) == 0) {
        return;
    }
    netTopicInfo[topic.topicName].openedParams.erase(topic.params);
    if (topic.topicName == OE_LOCAL_NET_AFFINITY) {
        if (topic.params == OE_PARA_PROCESS_AFFINITY) {
            CloseNetFlow(topic.topicName);
        } else if (topic.params == OE_PARA_LOC_NET_AFFI_USER_DEBUG || topic.params == OE_PARA_LOC_NET_AFFI_KERN_DEBUG) {
            debugCtl[topic.params] = false;
        }
    }
    if (topic.topicName == OE_NET_THREAD_QUE_DATA) {
        if (topic.params == OE_PARA_THREAD_RECV_QUE_CNT) {
            CloseNetFlow(topic.topicName);
        } else if (topic.params == OE_PARA_NET_RECV_QUE_USER_DEBUG || topic.params == OE_PARA_NET_RECV_QUE_KERN_DEBUG) {
            debugCtl[topic.params] = false;
        }
    }
}

void NetInterface::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

static void UpdateNetIntfBaseInfo(std::unordered_map<int, NetIntfBaseInfo> &info)
{
    std::unordered_set<std::string> netIntf = GetNetInterface();
    for (const auto &name : netIntf) {
        int ifindex = if_nametoindex(name.c_str());
        info[ifindex] = NetIntfBaseInfo{ .ifindex = ifindex, .name = name, .operstate = GetOperState(name) };
    }
}

oeaware::Result NetInterface::Enable(const std::string &param)
{
    (void)param;
    UpdateNetIntfBaseInfo(netIntfBaseInfo);
    return oeaware::Result(OK);
}

void NetInterface::Disable()
{
}

void NetInterface::Run()
{
    for (const auto &it : netTopicInfo) {
        const auto &topic = it.first;
        const auto &info = it.second;
        auto now = std::chrono::high_resolution_clock::now();
        int interval = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - netTopicInfo[topic].timestamp).count();
        netTopicInfo[topic].timestamp = now;
        if (interval < period) {
            interval = period;
        }
        for (const auto &param : info.openedParams) {
            if (topic == OE_NETWORK_INTERFACE_BASE_TOPIC) {
                PublishBaseInfo(param);
            } else if (topic == OE_NETWORK_INTERFACE_DRIVER_TOPIC) {
                PublishDriverInfo(param);
            } else if (topic == OE_LOCAL_NET_AFFINITY) {
                PublishLocalNetAffiInfo(param);
            } else if (topic == OE_NET_THREAD_QUE_DATA) {
                PublishNetQueueInfo(param, interval);
            }
        }
    }
    netIntfBaseInfo.clear();
    UpdateNetIntfBaseInfo(netIntfBaseInfo);
}

void NetInterface::InitTopicInfo(const std::string &name)
{
    netTopicInfo[name].topicName = name;
    // different topic can have different params
    if (name == OE_NETWORK_INTERFACE_BASE_TOPIC || name == OE_NETWORK_INTERFACE_DRIVER_TOPIC) {
        netTopicInfo[name].supportParams = { "operstate_up", "operstate_all" };
    } else if (name == OE_LOCAL_NET_AFFINITY) {
        netTopicInfo[name].supportParams = { OE_PARA_PROCESS_AFFINITY,
            OE_PARA_LOC_NET_AFFI_USER_DEBUG, OE_PARA_LOC_NET_AFFI_KERN_DEBUG };
    } else if (name == OE_NET_THREAD_QUE_DATA) {
        netTopicInfo[name].supportParams = { OE_PARA_THREAD_RECV_QUE_CNT,
            OE_PARA_NET_RECV_QUE_USER_DEBUG, OE_PARA_NET_RECV_QUE_KERN_DEBUG };
    }
}

void NetInterface::PublishBaseInfo(const std::string &params)
{
    DataList dataList;
    oeaware::SetDataListTopic(&dataList, OE_NET_INTF_INFO, OE_NETWORK_INTERFACE_BASE_TOPIC, params);
    dataList.len = 1;
    dataList.data = new void *[1];
    NetIntfBaseDataList *data = new NetIntfBaseDataList;
    int cnt = 0;
    for (const auto &item : netIntfBaseInfo) {
        if (!IfTopicParamsData(params, item.second)) {
            continue;
        }
        cnt++;
    }
    if (cnt > 0) {
        data->base = new NetworkInterfaceData[cnt];
    }
    cnt = 0;
    for (const auto &item : netIntfBaseInfo) {
        if (!IfTopicParamsData(params, item.second)) {
            continue;
        }
        const std::string &name = item.second.name;
        std::size_t length = std::min(name.size(), sizeof(data->base[cnt].name) - 1);
        std::copy(name.begin(), name.begin() + length, data->base[cnt].name);
        data->base[cnt].name[length] = '\0';
        data->base[cnt].operstate = item.second.operstate;
        data->base[cnt].ifindex = item.second.ifindex;
        cnt++;
    }
    data->count = cnt;
    dataList.data[0] = data;
    Publish(dataList);
}

void NetInterface::PublishDriverInfo(const std::string &params)
{
    DataList dataList;
    oeaware::SetDataListTopic(&dataList, OE_NET_INTF_INFO, OE_NETWORK_INTERFACE_DRIVER_TOPIC, params);
    dataList.len = 1;
    dataList.data = new void *[1];
    NetIntfDriverDataList *data = new NetIntfDriverDataList;
    int cnt = 0;
    for (const auto &item : netIntfBaseInfo) {
        if (!IfTopicParamsData(params, item.second)) {
            continue;
        }
        cnt++;
    }
    if (cnt > 0) {
        data->driver = new NetworkInterfaceDriverData[cnt];
    }
    cnt = 0;
    for (const auto &item : netIntfBaseInfo) {
        if (!IfTopicParamsData(params, item.second)) {
            continue;
        }
        const std::string &name = item.second.name;
        struct ethtool_drvinfo drvinfo;
        if (!ReadEthtoolDrvinfo(name, drvinfo)) {
            continue;
        }
        std::size_t length = std::min(name.size(), sizeof(data->driver[cnt].name) - 1);
        std::copy(name.begin(), name.begin() + length, data->driver[cnt].name);
        data->driver[cnt].name[length] = '\0';
        strncpy_s(data->driver[cnt].driver, sizeof(data->driver[cnt].driver), drvinfo.driver, sizeof(drvinfo.driver));
        strncpy_s(data->driver[cnt].version, sizeof(data->driver[cnt].version), drvinfo.version, sizeof(drvinfo.version));
        strncpy_s(data->driver[cnt].fwVersion, sizeof(data->driver[cnt].fwVersion), drvinfo.fw_version, sizeof(drvinfo.fw_version));
        strncpy_s(data->driver[cnt].busInfo, sizeof(data->driver[cnt].busInfo), drvinfo.bus_info, sizeof(drvinfo.bus_info));
        cnt++;
    }
    data->count = cnt;
    dataList.data[0] = data;
    Publish(dataList);
}

void NetInterface::PublishLocalNetAffiInfo(const std::string &params)
{
    if (params != OE_PARA_PROCESS_AFFINITY) {
        return;
    }
    for (auto &item : netIntfBaseInfo) {
        // incremental update net dev hook
        if (!AttachTcProgram((struct net_flow_kernel*)netFlowCtl.skel, item.second.name, item.second.ifindex)) {
            ERROR(logger, "Failed to attach TC program to " << item.second.name);
        }
    }
    std::unordered_map<uint64_t, uint64_t> flowData;
    ReadFlow(flowData);
    if (flowData.empty()) {
        return;
    }
    DataList dataList;
    oeaware::SetDataListTopic(&dataList, OE_NET_INTF_INFO, OE_LOCAL_NET_AFFINITY, params);
    dataList.len = 1;
    dataList.data = new void *[1];
    ProcessNetAffinityDataList *data = new ProcessNetAffinityDataList;

    data->count = flowData.size();
    data->affinity = new ProcessNetAffinityData[data->count];
    int cnt = 0;
    for (auto it = flowData.begin(); it != flowData.end(); ++it) {
        uint64_t pidPair = it->first;
        data->affinity[cnt].pid1 = static_cast<uint32_t>(pidPair >> UINT32_BIT_LEN);
        data->affinity[cnt].pid2 = static_cast<uint32_t>(pidPair & UINT32_MASK);
        data->affinity[cnt].level = it->second;
        cnt++;
    }
    dataList.data[0] = data;
    Publish(dataList);
}

void NetInterface::PublishNetQueueInfo(const std::string &params, const int &interval)
{
    if (params != OE_PARA_THREAD_RECV_QUE_CNT) {
        return;
    }
    for (auto &item : netIntfBaseInfo) {
        // incremental update net dev hook
        if (!AttachTcProgram((struct net_flow_kernel*)netFlowCtl.skel, item.second.name, item.second.ifindex)) {
            ERROR(logger, "Failed to attach TC program to " << item.second.name);
        }
    }
    std::vector<QueueInfo> threadQueData;
    ReadNetQueue(threadQueData);
    if (threadQueData.empty()) {
        return;
    }
    DataList dataList;
    oeaware::SetDataListTopic(&dataList, OE_NET_INTF_INFO, OE_NET_THREAD_QUE_DATA, params);
    dataList.len = 1;
    dataList.data = new void *[1];
    NetThreadQueDataList *data = new NetThreadQueDataList;

    data->count = threadQueData.size();
    data->intervalMs = interval;
    data->queData = new NetThreadQueData[data->count];
    for (size_t i = 0;i< threadQueData.size(); ++i) {
        data->queData[i].pid = threadQueData[i].td.pid;
        data->queData[i].tid = threadQueData[i].td.tid;
        data->queData[i].ifIndex = threadQueData[i].ifIdx;
        data->queData[i].queueId = threadQueData[i].queueId - 1; // kernel queue id start from 1
        data->queData[i].times = threadQueData[i].times;
        data->queData[i].len = threadQueData[i].len;
    }
    dataList.data[0] = data;
    Publish(dataList);
}

bool NetInterface::AttachTcProgram(struct net_flow_kernel *obj, std::string name, int ifindex)
{
    // warning: don't count(name), because the nic may have the same name after being deleted
    if (netFlowCtl.netDevHooks.count(ifindex) != 0) {
        return true;
    }
    // add attach filter
    if (netIntfBaseInfo[ifindex].operstate == IF_OPER_DOWN) {
        return true;
    }

    DECLARE_LIBBPF_OPTS(bpf_tc_hook, tc_hook,
        .ifindex = ifindex,
        .attach_point = BPF_TC_INGRESS);

    DECLARE_LIBBPF_OPTS(bpf_tc_opts, tc_opts,
        .handle = 1,
        .priority = 1);

    int err = bpf_tc_hook_create(&tc_hook);
    if (err && err != -EEXIST) {
        ERROR(logger, "Failed to create TC hook for " << name << ": " << err);
        return false;
    }
    tc_opts.prog_fd = bpf_program__fd(obj->progs.tc_ingress);
    err = bpf_tc_attach(&tc_hook, &tc_opts);
    if (err) {
        ERROR(logger, "Failed to attach TC program to " << name << ": " << err << ": " << ifindex);
        return false;
    }
    NetDevHook hook = { .tcHook = tc_hook, .tcOpts = tc_opts, .ifindex = ifindex };
    netFlowCtl.netDevHooks.emplace(ifindex, hook);
    if (debugCtl[OE_PARA_LOC_NET_AFFI_USER_DEBUG] || debugCtl[OE_PARA_NET_RECV_QUE_USER_DEBUG]) {
        INFO(logger, "NetInterface::AttachTcProgram Attach TC program to " << name << ": " << ifindex);
    }
    return true;
}

bool NetInterface::OpenNetFlow(const std::string &topicName)
{
    if (!netFlowCtl.openTopic.empty()) {
        netFlowCtl.openTopic.insert(topicName);
        return true; // already open
    }

    int err;
    struct net_flow_kernel *obj = net_flow_kernel__open_and_load();
    if (!obj) {
        ERROR(logger, "Failed to open net_flow BPF object");
        return false;
    }
    err = net_flow_kernel__attach(obj);
    if (err) {
        ERROR(logger, "Failed to attach BPF programs: " << err);
        return false;
    }
    for (auto &item : netIntfBaseInfo) {
        if (!AttachTcProgram(obj, item.second.name, item.second.ifindex)) {
            ERROR(logger, "Failed to attach TC program to " << item.second.name);
        }
    }
    if (netFlowCtl.netDevHooks.empty()) {
        net_flow_kernel__destroy(obj);
        return false;
    }
    netFlowCtl.skel = obj;
    netFlowCtl.openTopic.insert(topicName);
    return true;
}

void NetInterface::ReadFlow(std::unordered_map<uint64_t, uint64_t> &flowData)
{
    struct net_flow_kernel *obj = (struct net_flow_kernel *)netFlowCtl.skel;
    struct SockKey key, nextKey;
    struct SockInfo value;
    int err;
    struct bpf_map *map = obj->maps.flowStats;
    int allkeyNum = 0;
    int validKeyNum = 0;

    err = bpf_map__get_next_key(map, NULL, &key, sizeof(SockKey));
    while (!err) {
        allkeyNum++;
        if (bpf_map__lookup_elem(map, &key, bpf_map__key_size(map), &value, bpf_map__value_size(map), 0)) {
            break;
        }
        if (debugCtl[OE_PARA_LOC_NET_AFFI_USER_DEBUG]) {
            INFO(logger, "NetInterface::ReadFlow: ("
                << inet_ntoa(*reinterpret_cast<struct in_addr *>(&key.localIp)) << ") "
                << key.localIp << ":" << key.localPort << " <-> ("
                << inet_ntoa(*reinterpret_cast<struct in_addr *>(&key.remoteIp)) << ") "
                << key.remoteIp << ":" << key.remotePort << " value: " << value.flow << ", "
                << value.client.comm << " " << value.client.pid << " <-> "
                << value.server.comm << " " << value.server.pid);
        }
        if (value.flow == 0 || value.client.pid == 0 || value.server.pid == 0 ||
            (value.client.pid == value.server.pid)) {
            err = bpf_map__get_next_key(map, &key, &nextKey, sizeof(SockKey));
            key = nextKey;
            continue;
        }
        validKeyNum++;
        uint64_t lastFlow = netFlowCtl.lastSockFlow[key];
        // a new sock flow which has same 4 ntuple, the value.flow may small than lastFlow
        uint64_t diffFlow = value.flow > lastFlow ? value.flow - lastFlow : value.flow;
        netFlowCtl.lastSockFlow[key] = value.flow;
        // two pid as a unique key
        uint32_t pid1 = value.server.pid;
        uint32_t pid2 = value.client.pid;
        uint64_t pidPair = pid1 < pid2 ? static_cast<uint64_t>(pid1) << UINT32_BIT_LEN | static_cast<uint64_t>(pid2)
            : static_cast<uint64_t>(pid2) << UINT32_BIT_LEN | static_cast<uint64_t>(pid1);
        flowData[pidPair] += diffFlow;
        err = bpf_map__get_next_key(map, &key, &nextKey, sizeof(SockKey));
        key = nextKey;
    }
    if (debugCtl[OE_PARA_LOC_NET_AFFI_USER_DEBUG]) {
        INFO(logger, "ReadFlow allkeyNum: " << allkeyNum << ", validKeyNum: " << validKeyNum << ", flowData size: " << flowData.size());
    }
}

void NetInterface::ReadNetQueue(std::vector<QueueInfo> &threadQueData)
{
    struct net_flow_kernel *obj = (struct net_flow_kernel *)netFlowCtl.skel;
    struct SockKey key;
    struct SockKey nextKey;
    struct QueueInfo queInfo;
    int err;
    struct bpf_map *map = obj->maps.rcvQueStatus;
    int allkeyNum = 0;
    int validKeyNum = 0;

    std::unordered_map<ThreadQueKey, QueueInfo> threadQueMap; // every element is a unique (thread + queue)
    err = bpf_map__get_next_key(map, NULL, &key, sizeof(SockKey));
    while (!err) {
        allkeyNum++;
        if (bpf_map__lookup_elem(map, &key, bpf_map__key_size(map), &queInfo, bpf_map__value_size(map), 0)) {
            break;
        }
        if (debugCtl[OE_PARA_NET_RECV_QUE_USER_DEBUG]) {
            INFO(logger, "NetInterface::ReadNetQueue: ("
                << inet_ntoa(*reinterpret_cast<struct in_addr *>(&key.localIp)) << ") "
                << key.localIp << ":" << key.localPort << " <-> ("
                << inet_ntoa(*reinterpret_cast<struct in_addr *>(&key.remoteIp)) << ") "
                << key.remoteIp << ":" << key.remotePort << ", "
                << queInfo.td.comm << ", pid " << queInfo.td.pid << ", tid "
                << queInfo.td.tid << ", ifIdx " << queInfo.ifIdx << ", queId " << queInfo.queueId
                << ", times " << queInfo.times << ", len " << queInfo.len);
        }
        if (queInfo.ifIdx <= 0) {
            err = bpf_map__get_next_key(map, &key, &nextKey, sizeof(SockKey));
            key = nextKey;
            continue;
        }
        validKeyNum++;
        uint64_t lastQueTimes = netFlowCtl.lastQueTimes[key];
        uint64_t lastQueLen = netFlowCtl.lastQueLen[key];
        netFlowCtl.lastQueTimes[key] = queInfo.times;
        netFlowCtl.lastQueLen[key] = queInfo.len;
        // a new sock flow which has same 4 ntuple, the queInfo.times may small than lastQueTimes
        if (queInfo.times > lastQueTimes) {
            queInfo.times -= lastQueTimes;
            queInfo.len -= lastQueLen;
        }
        // aggregating elements of the same (tid, ifindex, queueId)
        ThreadQueKey queKey = { .tid = queInfo.td.tid, .ifindex = queInfo.ifIdx, .queueId = queInfo.queueId };
        if (!threadQueMap.count(queKey)) {
            threadQueMap[queKey] = queInfo;
        } else {
            threadQueMap[queKey].times += queInfo.times;
            threadQueMap[queKey].len += queInfo.len;
        }
        err = bpf_map__get_next_key(map, &key, &nextKey, sizeof(SockKey));
        key = nextKey;
    }
    for (const auto &it : threadQueMap) {
        threadQueData.emplace_back(it.second);
    }
    if (debugCtl[OE_PARA_NET_RECV_QUE_USER_DEBUG]) {
        INFO(logger, "ReadNetQueue allkeyNum: " << allkeyNum << ", validKeyNum: " << validKeyNum
            << ", threadQueData size: " << threadQueData.size());
    }
}

void NetInterface::CloseNetFlow(const std::string &topicName)
{
    netFlowCtl.openTopic.erase(topicName);
    if (!netFlowCtl.openTopic.empty()) {
        return;
    }
    struct net_flow_kernel *obj = (struct net_flow_kernel *)netFlowCtl.skel;

    for (auto &it : netFlowCtl.netDevHooks) {
        auto &hook = it.second;
        DECLARE_LIBBPF_OPTS(bpf_tc_opts, opts,
            .handle = hook.tcOpts.handle,
            .priority = hook.tcOpts.priority);
        opts.flags = opts.prog_fd = opts.prog_id = 0;
        int ret = bpf_tc_detach(&hook.tcHook, &opts);
        if (ret) {
            ERROR(logger, "Failed to detach TC from " << hook.tcHook.ifindex << ", ret: " << ret);
        }

        ret = bpf_tc_hook_destroy(&hook.tcHook);
        if (ret) {
            ERROR(logger, "Failed to destroy TC hook on " << hook.tcHook.ifindex << ", ret: " << ret);
        }
    }

    netFlowCtl.netDevHooks.clear();

    if (obj) {
        net_flow_kernel__detach(obj);
        net_flow_kernel__destroy(obj);
        netFlowCtl.skel = nullptr;
    }

    netFlowCtl.lastQueTimes.clear();
    netFlowCtl.lastQueLen.clear();
    netFlowCtl.lastSockFlow.clear();
}

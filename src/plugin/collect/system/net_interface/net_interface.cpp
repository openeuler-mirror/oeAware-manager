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
#include <arpa/inet.h>
#include <bpf/libbpf.h>
#include <bpf/libbpf_common.h>
#include <net/if.h>
#include <sys/resource.h>
#include "ebpf/net_flow.skel.h"

constexpr int UINT32_BIT_LEN = 32;
constexpr uint64_t UINT32_MASK = 0xFFFFFFFFULL;

struct ThreadData {
    uint32_t pid;
    uint32_t tid;
    char comm[16];
};

struct SockInfo {
    struct ThreadData client;
    struct ThreadData server;
    uint64_t flow;
};

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

    if (topic.topicName == OE_LOCAL_NET_AFFINITY && !OpenNetFlow()) {
        return oeaware::Result(FAILED, "open net flow failed");
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
    if (topic.topicName == OE_LOCAL_NET_AFFINITY) {
        CloseNetFlow();
    }
    netTopicInfo[topic.topicName].openedParams.erase(topic.params);
}

void NetInterface::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

oeaware::Result NetInterface::Enable(const std::string &param)
{
    (void)param;
    std::unordered_set<std::string> netIntf = GetNetInterface();
    for (auto &name : netIntf) {
        GetNetIntfBaseInfo(name, netIntfBaseInfo[name]);
    }
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
        for (const auto &param : info.openedParams) {
            if (topic == OE_NETWORK_INTERFACE_BASE_TOPIC) {
                PublishBaseInfo(param);
            } else if (topic == OE_NETWORK_INTERFACE_DRIVER_TOPIC) {
                PublishDriverInfo(param);
            } else if (topic == OE_LOCAL_NET_AFFINITY) {
                PublishLocalNetAffiInfo(param);
            }
        }
    }
}

void NetInterface::InitTopicInfo(const std::string &name)
{
    netTopicInfo[name].topicName = name;
    // different topic can have different params
    if (name == OE_NETWORK_INTERFACE_BASE_TOPIC || name == OE_NETWORK_INTERFACE_DRIVER_TOPIC) {
        netTopicInfo[name].supportParams = { "operstate_up", "operstate_all" };
    } else if (name == OE_LOCAL_NET_AFFINITY) {
        netTopicInfo[name].supportParams = { "process_affinity" };
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
    if (params != "process_affinity") {
        return;
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

bool NetInterface::OpenNetFlow()
{
    int err;
    struct net_flow_kernel *obj = net_flow_kernel__open_and_load();
    if (!obj) {
        ERROR(logger, "Failed to open BPF object");
        return false;
    }

    do {
        err = net_flow_kernel__attach(obj);
        if (err) {
            ERROR(logger, "Failed to attach BPF programs: " << err);
            break;
        }
        DECLARE_LIBBPF_OPTS(bpf_tc_hook, tc_hook, .ifindex = if_nametoindex("lo"),
            .attach_point = BPF_TC_INGRESS);
        DECLARE_LIBBPF_OPTS(bpf_tc_opts, tc_opts, .handle = 1, .priority = 1);
        err = bpf_tc_hook_create(&tc_hook);
        if (err && err != -EEXIST) {
            ERROR(logger, "Failed to bpf_tc_hook_create: " << err);
            break;
        }
        tc_opts.prog_fd = bpf_program__fd(obj->progs.tc_ingress);
        err = bpf_tc_attach(&tc_hook, &tc_opts);
        if (err) {
            ERROR(logger, "Failed to attach TC:  " << err);
            break;
        }
        skel = obj;
        return true;
    } while (0);

    net_flow_kernel__destroy(obj);
    return false;
}

void NetInterface::ReadFlow(std::unordered_map<uint64_t, uint64_t> &flowData)
{
    struct net_flow_kernel *obj = (struct net_flow_kernel *)skel;
    struct SockKey key, nextKey;
    struct SockInfo value;
    int err;
    struct bpf_map *map = obj->maps.flowStats;

    err = bpf_map__get_next_key(map, NULL, &key, sizeof(SockKey));
    while (!err) {
        if (bpf_map__lookup_elem(map, &key, bpf_map__key_size(map), &value, bpf_map__value_size(map), 0)) {
            break;
        }
        if (value.flow == 0) {
            continue;
        }
        uint64_t lastFlow = lastSockFlow[key];
        // a new sock flow which has same 4 ntuple, the value.flow may small than lastFlow
        uint64_t diffFlow = value.flow > lastFlow ? value.flow - lastFlow : value.flow;
        lastSockFlow[key] = value.flow;
        // two pid as a unique key
        uint32_t pid1 = value.server.pid;
        uint32_t pid2 = value.client.pid;
        uint64_t pidPair = pid1 < pid2 ? static_cast<uint64_t>(pid1) << UINT32_BIT_LEN | static_cast<uint64_t>(pid2)
            : static_cast<uint64_t>(pid2) << UINT32_BIT_LEN | static_cast<uint64_t>(pid1);
        flowData[pidPair] += diffFlow;
        err = bpf_map__get_next_key(map, &key, &nextKey, sizeof(SockKey));
        key = nextKey;
    }
}

void NetInterface::CloseNetFlow()
{
    struct net_flow_kernel *obj = (struct net_flow_kernel *)skel;
    DECLARE_LIBBPF_OPTS(bpf_tc_hook, tc_hook, .ifindex = if_nametoindex("lo"),
        .attach_point = BPF_TC_INGRESS);
    DECLARE_LIBBPF_OPTS(bpf_tc_opts, tc_opts, .handle = 1, .priority = 1);

    int ret = bpf_tc_detach(&tc_hook, &tc_opts);
    ret = bpf_tc_hook_destroy(&tc_hook);
    net_flow_kernel__detach((struct net_flow_kernel *)skel);
    net_flow_kernel__destroy((struct net_flow_kernel *)skel);
    skel = nullptr;
}

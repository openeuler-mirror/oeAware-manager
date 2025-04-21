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
        if (info.openedParams.empty()) {
            continue;
        }
        for (const auto &param : info.openedParams) {
            if (topic == OE_NETWORK_INTERFACE_BASE_TOPIC) {
                PublishBaseInfo(param);
            } else if (topic == OE_NETWORK_INTERFACE_DRIVER_TOPIC) {
                PublishDriverInfo(param);
            }
        }
    }
}

void NetInterface::InitTopicInfo(const std::string &name)
{
    // different topic can have different params
    if (name == OE_NETWORK_INTERFACE_BASE_TOPIC || name == OE_NETWORK_INTERFACE_DRIVER_TOPIC) {
        netTopicInfo[name].topicName = name;
        netTopicInfo[name].supportParams = { "operstate_up", "operstate_all" };
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

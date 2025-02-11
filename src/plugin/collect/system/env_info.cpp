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

#include "env_info.h"
#include <numa.h>
#include <unistd.h>
#include "oeaware/data/env_data.h"

using namespace oeaware;

void ResetEnvStaticInfo(EnvStaticInfo &envStaticInfo)
{
    envStaticInfo.numaNum = 0;
    envStaticInfo.cpuNumConfig = 0;
    envStaticInfo.pageSize = 0;
    envStaticInfo.pageMask = 0;
    if (envStaticInfo.cpu2Node) {
        delete[] envStaticInfo.cpu2Node;
        envStaticInfo.cpu2Node = nullptr;
    }
    if (envStaticInfo.numaDistance) {
        for (int i = 0; i < envStaticInfo.numaNum; i++) {
            if (envStaticInfo.numaDistance[i]) {
                delete[] envStaticInfo.numaDistance[i];
                envStaticInfo.numaDistance[i] = nullptr;
            }
        }
        delete[] envStaticInfo.numaDistance;
        envStaticInfo.numaDistance = nullptr;
    }
}
void ResetEnvRealTimeInfo(EnvRealTimeInfo &envRealTimeInfo)
{
    envRealTimeInfo.dataReady = ENV_DATA_NOT_READY;
    envRealTimeInfo.cpuNumOnline = 0;
}

EnvInfo::EnvInfo()
{
    name = OE_ENV_INFO;
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
    }
    description += "[introduction] \n";
    description += "               Collect information of environment \n";
    description += "[version] \n";
    description += "         " + version + "\n";
    description += "[instance name]\n";
    description += "                 " + name + "\n";
    description += "[instance period]\n";
    description += "                 " + std::to_string(period) + " ms\n";
    description += "[provide topics]\n";
    description += "                 none\n";
    description += "[running require arch]\n";
    description += "                      aarch64 x86\n";
    description += "[running require environment]\n";
    description += "                 kernel version(4.19, 5.10, 6.6)\n";
    description += "[running require topics]\n";
    description += "                        none\n";
	description += "[provide topics]\n";
	description += "                 topicName:static, params:\"\", usage:get static environment info\n";
	description += "                 topicName:realtime, params:\"\", usage:get realtime environment info\n";
}

oeaware::Result EnvInfo::OpenTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name) {
        return oeaware::Result(FAILED, "OpenTopic:" + topic.GetType() + " failed, instanceName is not match");
    }

    if (topic.params != "") {
        return oeaware::Result(FAILED, "OpenTopic:" + topic.GetType() + " failed, params is not empty");
    }
    for (auto &iter : topicStr) {
        if (iter != topic.topicName) {
            continue;
        }
        if (topicParams[iter].open) {
            WARN(logger, topic.GetType() << " has been opened before!");
            return oeaware::Result(OK);
        }
        topicParams[iter].open = true;
        return oeaware::Result(OK);
    }
    return oeaware::Result(OK);
}

void EnvInfo::CloseTopic(const oeaware::Topic &topic)
{
    if (!topicParams[topic.topicName].open) {
        WARN(logger, "CloseTopic failed, " + topic.GetType() + " not open.");
        return;
    }
    topicParams[topic.topicName].open = false;
    if (topic.topicName == "realtime") {
        ResetEnvRealTimeInfo(envRealTimeInfo);
    }
}

void EnvInfo::UpdateData(const DataList &dataList)
{
    (void)dataList;
    return;
}

oeaware::Result EnvInfo::Enable(const std::string &param)
{
    (void)param;
    // other topic may be depend on env static info, so init env static info first
    if (!InitEnvStaticInfo()) {
        return oeaware::Result(FAILED, "Enable failed, InitEnvStaticInfo failed.");
    }
    return oeaware::Result(OK);
}

void EnvInfo::Disable()
{
    ResetEnvStaticInfo(envStaticInfo);
    return;
}

void EnvInfo::Run()
{
    for (auto &topicName : topicStr) {
        if (!topicParams[topicName].open) {
            continue;
        }
        DataList dataList;
        if (topicName == "static") {
            oeaware::SetDataListTopic(&dataList, name, topicName, "");
            dataList.len = 1;
            dataList.data = new void* [1];
            dataList.data[0] = &envStaticInfo; // not need new data
        } else if (topicName == "realtime") {
            GetEnvRealtimeInfo();
            oeaware::SetDataListTopic(&dataList, name, topicName, "");
            dataList.len = 1;
            dataList.data = new void* [1];
            dataList.data[0] = &envRealTimeInfo;
        }
        Publish(dataList);
    }
}

unsigned long GetPageMask(const int &pageSize)
{
    unsigned long pageMask;
    if (pageSize > 0) {
        pageMask = ~(static_cast<unsigned long>(pageSize) - 1);
    } else {
        pageMask = ~0xFFF;
    }

    return pageMask;
}

void EnvInfo::InitNumaDistance()
{
    int numaNum = envStaticInfo.numaNum;
    envStaticInfo.numaDistance = new int *[numaNum];
    for (int i = 0; i < numaNum; ++i) {
        envStaticInfo.numaDistance[i] = new int[numaNum];
    }

    for (int i = 0; i < numaNum; ++i) {
        for (int j = 0; j < numaNum; ++j) {
            envStaticInfo.numaDistance[i][j] = numa_distance(i, j);
        }
    }
}

bool EnvInfo::InitEnvStaticInfo()
{
    envStaticInfo.numaNum = numa_num_configured_nodes();
    envStaticInfo.cpuNumConfig = sysconf(_SC_NPROCESSORS_CONF);
    if (envStaticInfo.numaNum < 0 || envStaticInfo.cpuNumConfig <= 0) {
        return false;
    }
    envStaticInfo.cpu2Node = new int[envStaticInfo.cpuNumConfig];
    struct bitmask *cpumask = numa_allocate_cpumask();
    for (int nid = 0; nid < envStaticInfo.numaNum; ++nid) {
        numa_bitmask_clearall(cpumask);
        numa_node_to_cpus(nid, cpumask);
        for (int cpu = 0; cpu < envStaticInfo.cpuNumConfig; cpu++) {
            if (numa_bitmask_isbitset(cpumask, cpu)) {
                envStaticInfo.cpu2Node[cpu] = nid;
            }
        }
    }
    numa_free_nodemask(cpumask);

    envStaticInfo.pageSize = sysconf(_SC_PAGE_SIZE);
    envStaticInfo.pageMask = GetPageMask(envStaticInfo.pageSize);
    InitNumaDistance();
    return true;
}

void EnvInfo::GetEnvRealtimeInfo()
{
    envRealTimeInfo.cpuNumOnline = sysconf(_SC_NPROCESSORS_ONLN);
    if (envRealTimeInfo.cpuNumOnline <= 0 || envRealTimeInfo.cpuNumOnline > envStaticInfo.cpuNumConfig) {
        envRealTimeInfo.dataReady = ENV_DATA_NOT_READY;
        return;
    }
    envRealTimeInfo.dataReady = ENV_DATA_READY;
}

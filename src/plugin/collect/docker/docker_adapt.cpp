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
#include "docker_adapt.h"
#include <iostream>
#include <securec.h>
#include <chrono>

constexpr int PERIOD = 500;
constexpr int PRIORITY = 0;

static bool GetContainerTasks(const std::string& containerId, std::vector<int32_t>& tasks)
{
    std::string fileName = "/sys/fs/cgroup/cpu/docker/" + containerId + "/tasks";
    std::ifstream file(fileName);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        int64_t tid = stoi(line);
        tasks.emplace_back(tid);
    }
    return true;
}

static bool GetContainersCpuInfo(int64_t &val, const std::string &containerId, const std::string &element)
{
    std::string fileName = "/sys/fs/cgroup/cpu/docker/" + containerId + "/" + element;
    std::ifstream file(fileName);
    if (!file.is_open()) {
        return false;
    }

    if (!(file >> val)) {
        return false;
    }
    return true;
}

static bool GetContainersCpusetInfo(std::string &val, const std::string &containerId, const std::string &element)
{
    std::string fileName = "/sys/fs/cgroup/cpuset/docker/" + containerId + "/" + element;
    std::ifstream file(fileName);
    if (!file.is_open()) {
        return false;
    }

    if (!(file >> val)) {
        return false;
    }
    return true;
}

static bool GetSystemCpuStatInfo(uint64_t &totalCpuStat)
{
    std::ifstream statFile("/proc/stat");
    if (!statFile.is_open()) {
        return false;
    }
    std::string skip;
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
    uint64_t guest;
    uint64_t guestNice;
    statFile >> skip >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guestNice;
    totalCpuStat = user + nice + system + idle + iowait + irq + softirq + steal;
    return true;
}

DockerAdapt::DockerAdapt()
{
    name = OE_DOCKER_COLLECTOR;
    description = "collect information of docker";
    version = "1.0.0";
    period = PERIOD;
    priority = PRIORITY;
    type = 0;

    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = this->name;
    supportTopics.push_back(topic);
}

oeaware::Result DockerAdapt::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    openStatus = true;
    return oeaware::Result(OK);
}

void DockerAdapt::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
    openStatus = false;
}

void DockerAdapt::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

oeaware::Result DockerAdapt::Enable(const std::string &param)
{
    (void)param;
    return oeaware::Result(OK);
}

void DockerAdapt::Disable()
{
    openStatus = false;
}

void DockerAdapt::Run()
{
    if (!openStatus) {
        return;
    }
    DockerCollect();
    DataList dataList;
    dataList.topic.instanceName = new char[name.size() + 1];
    strcpy_s(dataList.topic.instanceName, name.size() + 1, name.data());
    dataList.topic.topicName = new char[name.size() + 1];
    strcpy_s(dataList.topic.topicName, name.size() + 1, name.data());
    dataList.topic.params = new char[1];
    dataList.topic.params[0] = 0;
    dataList.data = new void* [containers.size()];
    uint64_t i = 0;
    for (auto &it : containers) {
        dataList.data[i++] = &it.second;
    }
    dataList.len = i;
    Publish(dataList, false);
}

void DockerAdapt::GetSamplingTimestamp(Container &container)
{
    using namespace std::chrono;
    const auto curTs = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
    container.sampling_timestamp = curTs;
}

void DockerAdapt::DockerUpdate(const std::unordered_set<std::string> &directories)
{
    uint64_t usage;
    // delete non-existent container
    for (auto it = containers.begin(); it != containers.end();) {
        if (directories.find(it->first) == directories.end()) {
            it = containers.erase(it);
        } else {
            ++it;
        }
    }
    // update/add container
    GetSystemCpuStatInfo(usage);
    for (const auto &dir : directories) {
        Container container;
        container.id = dir;
        bool ret = true;
        ret &= GetContainersCpuInfo(container.cfs_period_us, dir, "cpu.cfs_period_us");
        ret &= GetContainersCpuInfo(container.cfs_quota_us, dir, "cpu.cfs_quota_us");
        ret &= GetContainersCpuInfo(container.cfs_burst_us, dir, "cpu.cfs_burst_us");
        ret &= GetContainersCpusetInfo(container.cpus, dir, "cpuset.cpus");
        ret &= GetContainersCpusetInfo(container.mems, dir, "cpuset.mems");
        ret &= GetContainerTasks(dir, container.tasks);
        ret &= GetContainersCpuInfo(container.cpu_usage, dir, "cpuacct.usage");
        container.system_cpu_usage = usage;
        GetSamplingTimestamp(container);
        container.soft_quota = -1;
        GetContainersCpuInfo(container.soft_quota, dir, "cpu.soft_quota");
        if (ret) {
            containers[dir] = container;
        }
    }
}

void DockerAdapt::DockerCollect()
{
    std::unordered_set<std::string> sub_dir;
    DIR *dir = opendir("/sys/fs/cgroup/cpu/docker/");
    if (dir != nullptr) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                sub_dir.insert(std::string(entry->d_name));
            }
        }
        closedir(dir);
    }
    DockerUpdate(sub_dir);
}
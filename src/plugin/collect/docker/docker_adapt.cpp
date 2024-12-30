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
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <csignal>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <securec.h>

constexpr int PERIOD = 1000;
constexpr int PRIORITY = 0;

static bool GetContainersInfo(int64_t &val, const std::string &container_id, const std::string &element)
{
    std::string file_name = "/sys/fs/cgroup/cpu/docker/" + container_id +"/" +element;
    std::ifstream file(file_name);
    if (!file.is_open()) {
        return false;
    }

    if (!(file >> val)) {
        return false;
    }
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
    Publish(dataList);
}

void DockerAdapt::DockerUpdate(const std::unordered_set<std::string> &directories)
{
    // delete non-existent container
    for (auto it = containers.begin(); it != containers.end();) {
        if (directories.find(it->first) == directories.end()) {
            it = containers.erase(it);
        } else {
            ++it;
        }
    }
    // update/add container
    for (const auto &dir : directories) {
        Container tmp;
        bool read_success = true;
        read_success &= GetContainersInfo(tmp.cfs_period_us, dir, "cpu.cfs_period_us");
        read_success &= GetContainersInfo(tmp.cfs_quota_us, dir, "cpu.cfs_quota_us");
        read_success &= GetContainersInfo(tmp.cfs_burst_us, dir, "cpu.cfs_burst_us");
        if (read_success) {
            Container container;
            container.cfs_period_us = tmp.cfs_period_us;
            container.cfs_quota_us = tmp.cfs_quota_us;
            container.cfs_burst_us = tmp.cfs_burst_us;
            container.id = dir;
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
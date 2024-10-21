/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/

#include "thread_collector.h"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <csignal>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>



void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<ThreadCollector>());
}

ThreadCollector::ThreadCollector() {
    name = "thread_collector";
    description = "collect information of thread";
    version = "1.0.0";
    period = 500;
    priority = 0;

    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = this->name;
    supportTopics.push_back(topic);
}

int ThreadCollector::OpenTopic(const oeaware::Topic &topic) {
    openStatus = true;
    return 0;
}

void ThreadCollector::CloseTopic(const oeaware::Topic &topic) {
    openStatus = false;
}

void ThreadCollector::UpdateData(const oeaware::DataList &dataList) {
    (void)dataList;
}

int ThreadCollector::Enable(const std::string &param) {
    (void)param;
    return 0;
}

void ThreadCollector::Disable() {
    openStatus = false;
}

void ThreadCollector::Run() {
    if (!openStatus) return;
    GetAllThreads();
    oeaware::DataList dataList;
    dataList.topic = supportTopics[0];
    for (auto &it : threads) {
        dataList.data.push_back(it.second);
    }
    Publish(dataList);
}

void ThreadCollector::GetAllThreads() {
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == nullptr) {
        return ;
    }
    ClearInvalidThread();
    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (!isdigit(entry->d_name[0])) {
            continue;
        }
        int pid = atoi(entry->d_name);
        std::string task_path = "/proc/" + std::to_string(pid) + "/task";
        DIR *task_dir = opendir(task_path.c_str());
        if (task_dir == nullptr) {
            continue;
        }
        struct stat task_stat;
        /* Continue if the process does not change */
        if (IsNotChange(&task_stat, task_path, pid)) {
            closedir(task_dir);
            continue;
        }
        /* Update last modification time of the process. */
        taskTime[pid] = task_stat.st_mtime;
        /* Update threads info */
        CollectThreads(pid, task_dir);
        closedir(task_dir);

    }
    closedir(proc_dir);
}

bool ThreadCollector::IsNotChange(struct stat *task_stat, const std::string &task_path, int pid) {
    return stat(task_path.c_str(), task_stat) != 0 || (taskTime.count(pid) && taskTime[pid] == task_stat->st_mtime);
}

void ThreadCollector::CollectThreads(int pid, DIR *task_dir) {
    struct dirent *task_entry;
    while ((task_entry = readdir(task_dir)) != nullptr) {
        if (!isdigit(task_entry->d_name[0])) {
            continue;
        }
        int tid = atoi(task_entry->d_name);
        /* Update if the thread exists. */
        threads[tid] = GetThreadInfo(pid, tid);
    }
}

std::shared_ptr<ThreadInfo> ThreadCollector::GetThreadInfo(int pid, int tid) {
    std::string s_path = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid) + "/comm";
    std::ifstream input_file(s_path);
    if (!input_file) {
        return nullptr;
    }
    std::string name;
    input_file >> name;
    return std::make_shared<ThreadInfo>(pid, tid, name);
}

void ThreadCollector::ClearInvalidThread() {
    for (auto it = threads.begin(); it != threads.end(); ) {
        auto tid = it->first;
        std::string task_path = "/proc/" + std::to_string(it->second->pid) + "/task/" + std::to_string(it->second->tid);
        if (access(task_path.c_str(), F_OK) < 0) {
            if (taskTime.count(tid)) {
                taskTime.erase(tid);
            }
            it = threads.erase(it);
        }else {
            ++it;
        }
    }
}

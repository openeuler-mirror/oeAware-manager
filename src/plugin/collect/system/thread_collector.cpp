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
#include <securec.h>
#include "data_register.h"

ThreadCollector::ThreadCollector()
{
    name = OE_THREAD_COLLECTOR;
    description = "collect information of thread";
    version = "1.0.0";
    period = 500;
    priority = 0;
    type = 0;
    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = this->name;
    supportTopics.push_back(topic);
}

oeaware::Result ThreadCollector::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    openStatus = true;
    return oeaware::Result(OK);
}

void ThreadCollector::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
    openStatus = false;
}

void ThreadCollector::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

oeaware::Result ThreadCollector::Enable(const std::string &param)
{
    (void)param;
    return oeaware::Result(OK);
}

void ThreadCollector::Disable()
{
    openStatus = false;
    for (auto &item : threads) {
        oeaware::Register::GetInstance().GetDataFreeFunc(OE_THREAD_COLLECTOR)(item.second);
    }
    threads.clear();
    taskTime.clear();
}

void ThreadCollector::Run()
{
    if (!openStatus) return;
    GetAllThreads();
    DataList dataList;
    dataList.topic.instanceName = new char[name.size() + 1];
    strcpy_s(dataList.topic.instanceName, name.size() + 1, name.data());
    dataList.topic.topicName = new char[name.size() + 1];
    strcpy_s(dataList.topic.topicName, name.size() + 1, name.data());
    dataList.topic.params = new char[1];
    dataList.topic.params[0] = 0;
    dataList.data = new void* [threads.size()];
    uint64_t i = 0;
    for (auto &it : threads) {
        auto info = new ThreadInfo();
        info->pid = it.second->pid;
        info->tid = it.second->tid;
        info->name = new char[strlen(it.second->name) + 1];
        strcpy_s(info->name, strlen(it.second->name) + 1, it.second->name);
        dataList.data[i++] = info;
    }
    dataList.len = i;
    Publish(dataList);
}

void ThreadCollector::GetAllThreads()
{
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

bool ThreadCollector::IsNotChange(struct stat *taskStat, const std::string &taskPath, int pid)
{
    return stat(taskPath.c_str(), taskStat) != 0 || (taskTime.count(pid) && taskTime[pid] == taskStat->st_mtime);
}

void ThreadCollector::CollectThreads(int pid, DIR *taskDir)
{
    struct dirent *taskEntry;
    while ((taskEntry = readdir(taskDir)) != nullptr) {
        if (!isdigit(taskEntry->d_name[0])) {
            continue;
        }
        int tid = atoi(taskEntry->d_name);
        /* Update if the thread exists. */
        auto info = GetThreadInfo(pid, tid);
        if (info == nullptr) {
            continue;
        }
        threads[tid] = info;
    }
}

ThreadInfo* ThreadCollector::GetThreadInfo(int pid, int tid)
{
    std::string s_path = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid) + "/comm";
    std::ifstream inputFile(s_path);
    if (!inputFile) {
        return nullptr;
    }
    auto threadInfo = new ThreadInfo();
    threadInfo->pid = pid;
    threadInfo->tid = tid;
    std::string name;
    inputFile >> name;
    threadInfo->name = new char[name.size() + 1];
    strcpy_s(threadInfo->name, name.size() + 1, name.data());
    return threadInfo;
}

void ThreadCollector::ClearInvalidThread()
{
    for (auto it = threads.begin(); it != threads.end(); ) {
        auto tid = it->first;
        std::string taskPath = "/proc/" + std::to_string(it->second->pid) + "/task/" + std::to_string(it->second->tid);
        if (access(taskPath.c_str(), F_OK) < 0) {
            if (taskTime.count(tid)) {
                taskTime.erase(tid);
            }
            delete[] it->second->name;
            delete it->second;
            it = threads.erase(it);
        }else {
            ++it;
        }
    }
}

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

#if ENABLE_EBPF
std::vector<struct thread_event*> ThreadCollector::threadEvents;
#endif

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
#if ENABLE_EBPF
    // 先初始化一次所有的线程信息，然后打开ebpf追踪线程变化情况
    INFO(logger, "enable ebpf thread collector");
    GetAllThreads();
    return OpenThreadTrace();
#endif
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
#if ENABLE_EBPF
    CloseThreadTrace();
    threadEvents.clear();
#endif
}

void ThreadCollector::Run()
{
    if (!openStatus) return;
#if ENABLE_EBPF
    ReadThreadTrace();
    ParseThreadEvent();
    threadEvents.clear();
#else
    GetAllThreads();
#endif
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
        DEBUG(logger, "thread info: pid=" << info->pid << ", tid=" << info->tid << ", name=" << info->name);
    }
    dataList.len = i;
    Publish(dataList);
}

#if ENABLE_EBPF
oeaware::Result ThreadCollector::OpenThreadTrace()
{
    skel = thread_collector_bpf__open_and_load();
    if (!skel) {
        ERROR(logger, "Failed to open and load bpf program");
        return oeaware::Result(FAILED);
    }

    int err = thread_collector_bpf__attach(skel);
    if (err) {
        ERROR(logger, "Failed to attach bpf program, " << err);
        thread_collector_bpf__destroy(skel);
        skel = nullptr;
        return oeaware::Result(FAILED);
    }

    rb = ring_buffer__new(bpf_map__fd(skel->maps.thread_events), handle_event, NULL, NULL);
    if (!rb) {
        ERROR(logger, "Failed to create ring buffer, " << -errno);
        thread_collector_bpf__destroy(skel);
        skel = nullptr;
        return oeaware::Result(FAILED);
    }

    return oeaware::Result(OK);
}

oeaware::Result ThreadCollector::ReadThreadTrace()
{
    int err = 0;
    err = ring_buffer__poll(rb, 0);
    if (err < 0) {
        ERROR(logger, "Failed to poll ring buffer, " << err);
        return oeaware::Result(FAILED);
    }
    return oeaware::Result(OK);
}

void ThreadCollector::ParseThreadEvent()
{
    for (auto &event : threadEvents) {
        if (event->is_create) {
            auto info = RecordThreadInfo(event->pid, event->tid, event->comm);
            if (info != nullptr) {
                threads[event->tid] = info;
            }
        } else {
            threads.erase(event->tid);
        }
    }
}

void ThreadCollector::CloseThreadTrace()
{
    ring_buffer__free(rb);
    thread_collector_bpf__destroy(skel);
    skel = nullptr;
}
#endif

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
#if !ENABLE_EBPF
        struct stat task_stat;
        /* Continue if the process does not change */
        if (IsNotChange(&task_stat, task_path, pid)) {
            closedir(task_dir);
            continue;
        }
        /* Update last modification time of the process. */
        taskTime[pid] = task_stat.st_mtime;
#endif
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

ThreadInfo* ThreadCollector::RecordThreadInfo(int pid, int tid, char comm[])
{
    auto threadInfo = new ThreadInfo();
    threadInfo->pid = pid;
    threadInfo->tid = tid;
    threadInfo->name = new char[strlen(comm) + 1];
    strcpy_s(threadInfo->name, strlen(comm) + 1, comm);
    return threadInfo;
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

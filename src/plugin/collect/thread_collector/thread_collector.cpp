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
#include "interface.h"
#include "thread_info.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <csignal>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

char thread_name[] = "thread_collector";
const int CYCLE_SIZE = 500;
static DataRingBuf ring_buf;
static DataBuf data_buf;
static ThreadInfo threads[THREAD_NUM];
/* For quickly access to the threads array. key: tid, value: the index of threads[THREAD_NUM]. */
static std::unordered_map<int, int> tids;
/* Saves the last modification time of the task dir in process. */
static std::unordered_map<int, long int> task_time;

static void clear_invalid_tid(int &num) {
    int cur = -1;

    for (int i = 0; i < num; ++i) {
        auto tid = threads[i].tid;
        std::string task_path = "/proc/" + std::to_string(threads[i].pid) + "/task/" + std::to_string(threads[i].tid);
        /* If current thread does not exist, clear it. */
        if (access(task_path.c_str(), F_OK) < 0) {
            tids.erase(tid);
            if (task_time.count(tid)) {
                task_time.erase(tid);
            }
            /* Find the first invalid position. */
            if (cur == -1) {
                cur = i;
            }
            continue;
        }
        /* Update threads by moving threads in the back to the front */
        if (cur != -1) {
            threads[cur++] = threads[i];
        }
    }
    /* Update thread total number. */
    if (cur != -1) {
        num = cur;
    }
}

static ThreadInfo get_thread_info(int pid, int tid) {
    std::string s_path = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid) + "/comm";
    std::ifstream input_file(s_path); 
    if (!input_file) {
        return ThreadInfo{};
    }
    std::string name;
    input_file >> name;
    return ThreadInfo{pid, tid, name};
}

static bool process_not_change(struct stat *task_stat, const std::string &task_path, int pid) {
    return stat(task_path.c_str(), task_stat) != 0 || (task_time.count(pid) && task_time[pid] == task_stat->st_mtime);
}

static void collect_threads(int pid, DIR *task_dir, int &num) {
    struct dirent *task_entry;
    while ((task_entry = readdir(task_dir)) != nullptr) {
        if (!isdigit(task_entry->d_name[0])) {
            continue;
        }
        int tid = atoi(task_entry->d_name);
        /* Update if the thread exists. */
        if (tids.count(tid)) {
            threads[tids[tid]] = get_thread_info(pid, tid);
            continue;
        }
        /* If the thread does not exist, add it. */
        if (num < THREAD_NUM) {
            tids[tid] = num;
            threads[num++] = get_thread_info(pid, tid);
        }
    }
}

static int get_all_threads(int &num) {
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == nullptr) {
        return 0;
    }
    clear_invalid_tid(num);
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
        if (process_not_change(&task_stat, task_path, pid)) {
            closedir(task_dir);
            continue;
        }
        /* Update last modification time of the process. */
        task_time[pid] = task_stat.st_mtime;
        /* Update threads info */
        collect_threads(pid, task_dir, num); 
        closedir(task_dir);
        
    }
    closedir(proc_dir);
    return num;
}

const char* get_name() {
    return thread_name;
}

const char* get_version() {
    return nullptr;
}

const char* get_description() {
    return nullptr;
}

int get_period() {
    return CYCLE_SIZE;
}
int get_priority() {
    return 0;
}
bool enable() {
    tids.clear();
    task_time.clear();
    ring_buf.count = 0;
    ring_buf.index = -1;
    ring_buf.buf_len = 1;
    ring_buf.buf = &data_buf; 
    return true;
}

void disable() {
    
}

const DataRingBuf* get_ring_buf() {
    return &ring_buf;
}

void run(const Param *param) {
    (void)param;
    ring_buf.count++;
    int index = (ring_buf.index + 1) % ring_buf.buf_len;
    int num = data_buf.len;
    get_all_threads(num);
    data_buf.len = num;
    data_buf.data = (void*)threads;
    ring_buf.buf[index] = data_buf;
    ring_buf.index = index;
}

struct Interface thread_collect = {
    .get_version = get_version,
    .get_name = get_name,
    .get_description = get_description,
    .get_dep = nullptr,
    .get_priority = get_priority,
    .get_type = nullptr,
    .get_period = get_period,
    .enable = enable,
    .disable = disable,
    .get_ring_buf = get_ring_buf,
    .run = run,
};

extern "C" int get_instance(Interface **ins) {
    *ins = &thread_collect;
    return 1;
}

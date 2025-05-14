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
#ifndef OEAWARE_MANAGER_THREAD_COLLECTOR_H
#define OEAWARE_MANAGER_THREAD_COLLECTOR_H
#include <unordered_map>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <linux/version.h>
#include "oeaware/interface.h"
#include "oeaware/data/thread_info.h"
#if ENABLE_EBPF
#include "ebpf/thread_collector.skel.h"
#endif

#define TASK_COMM_LEN 16

struct thread_event {
    uint64_t timestamp;
    uint32_t pid;
    uint32_t tid;
    char comm[TASK_COMM_LEN];
    bool is_create;
};

class ThreadCollector: public oeaware::Interface {
public:
    ThreadCollector();
    ~ThreadCollector() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    void GetAllThreads();
    bool IsNotChange(struct stat *task_stat, const std::string &task_path, int pid);
    void CollectThreads(int pid, DIR *task_dir);
    void ClearInvalidThread();
    ThreadInfo* GetThreadInfo(int pid, int tid);
    ThreadInfo* RecordThreadInfo(int pid, int tid, char comm[]);
    bool openStatus = false;
    std::unordered_map<int, ThreadInfo*> threads {};
    std::unordered_map<int, long int> taskTime {};

#if ENABLE_EBPF
    oeaware::Result OpenThreadTrace();
    void CloseThreadTrace();
    oeaware::Result ReadThreadTrace();
    void ParseThreadEvent();

    struct thread_collector_bpf *skel = nullptr;
    struct ring_buffer *rb = nullptr;
    static std::vector<struct thread_event*> threadEvents;

    static int handle_event(void *ctx, void *data, size_t size) {
        struct thread_event *event = (struct thread_event *)data;
        ThreadCollector::threadEvents.push_back(event);
        return 0;
    }
#endif
};
#endif //OEAWARE_MANAGER_THREAD_COLLECTOR_H

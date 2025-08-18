/******************************************************************************
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#define TASK_COMM_LEN 16

struct thread_event {
    uint64_t timestamp;
    uint32_t pid;
    uint32_t tid;
    char comm[TASK_COMM_LEN];
    bool is_create;
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);
} thread_events SEC(".maps");

// 监控线程创建
SEC("tracepoint/syscalls/sys_exit_clone")
int trace_sys_exit_clone(struct trace_event_raw_sys_exit *ctx)
{
    struct thread_event *event;
    
    event = bpf_ringbuf_reserve(&thread_events, sizeof(struct thread_event), 0);
    if (!event) {
        return 0;
    }

    long ret_val = ctx->ret;
    bpf_core_read(&ret_val, sizeof(ret_val), &ctx->ret);

    // if not wrong
    if (ret_val) {
        event->timestamp = bpf_ktime_get_ns();
        event->pid = bpf_get_current_pid_tgid() >> 32;
        event->tid = (u32)ret_val;
        event->is_create = true;
        bpf_get_current_comm(event->comm, sizeof(event->comm));
    }

    bpf_ringbuf_submit(event, 0);
    return 0;
}

// 监控线程销毁
SEC("tracepoint/syscalls/sys_enter_exit")
int trace_sys_enter_exit(struct trace_event_raw_sys_exit *ctx)
{
    struct thread_event *event;
    event = bpf_ringbuf_reserve(&thread_events, sizeof(struct thread_event), 0);
    if (!event) {
        return 0;
    }

    event->timestamp = bpf_ktime_get_ns();
    event->pid = bpf_get_current_pid_tgid() >> 32;
    event->tid = bpf_get_current_pid_tgid() & 0xffffffff;
    event->is_create = false;
    bpf_get_current_comm(event->comm, sizeof(event->comm));

    bpf_ringbuf_submit(event, 0);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";

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
#include "thread_tune.h"
#include "interface.h"
#include <cstdbool>
#include <set>
#include <fstream>
#include <numa.h>
#include <sched.h>

#define MAX_CPU 4096
#define MAX_NODE 8
#define DEFAULT_BIND_NODE 0

struct Node {
    int id;
    cpu_set_t cpu_mask;
    int cpus[MAX_CPU];
    int cpu_num;
};

static int cpu_num = 0;
static int max_node = 0;
static int cpu_node_map[MAX_CPU];
static cpu_set_t all_cpumask;
static Node g_nodes[MAX_NODE];
static std::set<int> bind_tid;
static bool is_init = false;
const std::string CONFIG_PATH = "/usr/lib64/oeAware-plugin/thread_tune.conf";
static std::set<std::string> ket_set;

static void read_ket_set(const std::string &file_name)
{
    std::ifstream file(file_name);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        ket_set.insert(line);
    }
    file.close();
}

static void init_cpu_map()
{
    int nid;

    for (int i = 0; i < MAX_CPU; i++) {
        cpu_node_map[i] = -1;
    }

    for (int cpu = 0; cpu < cpu_num; cpu++) {
        nid = numa_node_of_cpu(cpu);
        cpu_node_map[cpu] = nid;
    }
}

static void init_node_cpu_mask()
{
    int cpu_index = 0;

    for (int i = 0; i <= max_node; i++) {
        Node *node = &g_nodes[i];
        node->id = i;
        CPU_ZERO(&node->cpu_mask);

        for (int cpu = 0; cpu < cpu_num; cpu++) {
            if (cpu_node_map[cpu] == node->id) {
                CPU_SET(cpu, &node->cpu_mask);
                node->cpus[cpu_index++] = cpu;
            }
        }

        node->cpu_num = cpu_index;
    }
}

static cpu_set_t *get_node_cpuset(int nid)
{
    return &g_nodes[nid].cpu_mask;
}

static void init_all_cpumask()
{
    CPU_ZERO(&all_cpumask);
    for (int cpu = 0; cpu < cpu_num; cpu++) {
        CPU_SET(cpu, &all_cpumask);
    }
}

static cpu_set_t *get_all_cpumask()
{
    return &all_cpumask;
}

const char *get_version()
{
    return nullptr;
}

const char *get_name()
{
    return "thread_tune";
}

const char *get_description()
{
    return nullptr;
}

const char *get_dep()
{
    return "thread_scenario";
}

int get_priority()
{
    return 2;
}

int get_period()
{
    return 100;
}

bool enable()
{
    if (!is_init) {
        // The static data such as the number of CPUs and NUMA nodes
        // only needs to be assigned when enabling for the first time.
        cpu_num = numa_num_configured_cpus();
        max_node = numa_max_node();
        init_cpu_map();
        init_node_cpu_mask();
        init_all_cpumask();
        is_init = true;
    }

    read_ket_set(CONFIG_PATH);
    return true;
}

void disable()
{
    cpu_set_t *mask = get_all_cpumask();
    
    for(const auto &tid : bind_tid) {
        sched_setaffinity(tid, sizeof(*mask), mask);
    }

    bind_tid.clear();
    ket_set.clear();
}

const struct DataRingBuf *get_ring_buf()
{
    return nullptr;
}

void run(const Param *param)
{
    if (param->len != 1) {
        return;
    }

    auto *header = param->ring_bufs[0];
    DataBuf buf = header->buf[header->count % header->buf_len];
    ThreadInfo *data = (ThreadInfo *)buf.data;
    int tid;

    // bind numa node0 by default.
    cpu_set_t *new_mask = get_node_cpuset(DEFAULT_BIND_NODE);
    cpu_set_t current_mask;

    for (int i = 0; i < buf.len; ++i) {
        tid = data[i].tid;
        if (tid == -1 || tid == 0) {
            continue;
        }

        if (ket_set.find(data[i].name) == ket_set.end()) {
            continue;
        }

        sched_getaffinity(tid, sizeof(current_mask), &current_mask);
        // If current_mask is equal to new_mask, no further action
        // is taken to avoid calls to sched_setaffinity redundantly.
        if (CPU_EQUAL(new_mask, &current_mask)) {
            continue;
        }

        if (sched_setaffinity(tid, sizeof(*new_mask), new_mask) == 0) {
            bind_tid.insert(tid);
        }

    }
}

static struct Interface tune_interface = {
    .get_version = get_version,
    .get_name = get_name,
    .get_description = get_description,
    .get_dep = get_dep,
    .get_priority = get_priority,
    .get_type = nullptr,
    .get_period = get_period,
    .enable = enable,
    .disable = disable,
    .get_ring_buf = get_ring_buf,
    .run = run,
};

extern "C" int get_instance(Interface **ins) {
    *ins = &tune_interface;
    return 1;
}

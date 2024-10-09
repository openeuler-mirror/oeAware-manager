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
#include "thread_aware.h"
#include "interface.h"
#include <vector>
#include <fstream>
#include <cstring>

char name[] = "thread_scenario";
char dep[] = "thread_collector";
const std::string CONFIG_PATH = "/usr/lib64/oeAware-plugin/thread_scenario.conf";
const int CYCLE_SIZE = 100;
static std::vector<ThreadInfo> thread_info(THREAD_NUM);
static DataRingBuf data_ring_buf;
static DataBuf data_buf;
static std::vector<std::string> key_list;

static void read_key_list(const std::string &file_name) {
    std::ifstream file(file_name);
    if (!file.is_open()) {
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        key_list.emplace_back(line);
    }
    file.close();
}

const char* get_version() {
    return nullptr;
}

const char* get_name() {
    return name;
}

const char* get_description() {
    return nullptr;
}

const char* get_dep() {
    return dep;
}

int get_period() {
    return CYCLE_SIZE;
}

int get_priority() {
    return 1;
}

bool enable() {
    data_ring_buf.index = -1;
    data_ring_buf.count = 0;
    data_ring_buf.buf_len = 1;
    data_ring_buf.buf = &data_buf;
    read_key_list(CONFIG_PATH);
    return true;
}

void disable() {
    key_list.clear();
}

void run(const Param *param) {
    if (param->len != 1) return;
    data_ring_buf.count++;
    int index = (data_ring_buf.index + 1) % data_ring_buf.buf_len;
    auto *header = param->ring_bufs[0];
    DataBuf buf = header->buf[header->count % header->buf_len];
    ThreadInfo *data = (ThreadInfo*)buf.data;
    int cnt = 0;
    for (int i = 0; i < buf.len; ++i) {
        for (size_t j = 0; j < key_list.size(); ++j) {
            if (data[i].name == key_list[j] && cnt < THREAD_NUM) {
                thread_info[cnt].name = data[i].name;
                thread_info[cnt].tid = data[i].tid;
                thread_info[cnt].pid = data[i].pid;
                cnt++;
                break;
            }
        }
    }
    data_buf.len = cnt;
    data_buf.data = thread_info.data();
    data_ring_buf.buf[index] = data_buf;
    data_ring_buf.index = index;
}

const DataRingBuf* get_ring_buf() {
    return &data_ring_buf;
}

static struct Interface aware_interface = {
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
    *ins = &aware_interface;
    return 1;
}

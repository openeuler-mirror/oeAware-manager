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
#include "stealtask_tune.h"
#include "interface.h"
#include <string>
#include <fstream>

static bool is_init = false;
static std::string CMDLINE_PATH = "/proc/cmdline";
static std::string cmdline;

static void read_config(const std::string &file_name, std::string &content)
{
    std::ifstream file(file_name);
    if (!file.is_open()) {
        return;
    }

    std::getline(file, content);
    file.close();
}

const char *get_version()
{
    return nullptr;
}

const char *get_name()
{
    return "stealtask_tune";
}

const char *get_description()
{
    return nullptr;
}

const char *get_dep()
{
    return nullptr;
}

int get_priority()
{
    return 2;
}

int get_period()
{
    return 1000;
}

bool enable()
{
    if (system("zcat /proc/config.gz | grep CONFIG_SCHED_STEAL=y > /dev/null") == 1) {
        return false;
    }

    if (!is_init) {
        read_config(CMDLINE_PATH, cmdline);
        std::ofstream file("/sys/kernel/debug/sched_features");
        file << "STEAL";
        file.close();
        is_init = true;
    }

    std::string::size_type pos = cmdline.find("sched_steal_node_limit");
    if (pos == std::string::npos) {
        return false;
    }

    return true;
}

void disable()
{
    std::ofstream file("/sys/kernel/debug/sched_features");
    file << "NO_STEAL";
    file.close();
    is_init = false;
    return;
}

const struct DataRingBuf *get_ring_buf()
{
    return nullptr;
}

void run(const Param *param)
{
    return;
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

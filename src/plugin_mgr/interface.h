/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/ 
#ifndef PLUGIN_MGR_INTERFACE_H
#define PLUGIN_MGR_INTERFACE_H

enum PluginType {
    COLLECTOR,
    SCENARIO,
    TUNE,
};

struct Param {
    void *args;
    int len;
};

struct Interface {
    const char* (*get_version)();
    const char* (*get_name)();
    const char* (*get_description)();
    const char* (*get_dep)();
    PluginType (*get_type)();
    int (*get_cycle)();
    bool (*enable)();
    void (*disable)();
    const void* (*get_ring_buf)();
    void (*run)(const Param*);
};

#endif // !PLUGIN_MGR_INTERFACE_H

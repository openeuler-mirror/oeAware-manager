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
#ifndef PLUGIN_MGR_INSTANCE_RUN_HANDLER_H
#define PLUGIN_MGR_INSTANCE_RUN_HANDLER_H

#include "safe_queue.h"
#include "plugin.h"
#include "logger.h"
#include "memory_store.h"
#include <set>
#include <string>
#include <vector>
#include <unordered_map>

enum class RunType {
    ENABLED,
    DISABLED,
 };

// Message for communication between plugin manager and instance scheduling
class InstanceRunMessage {
public:
    InstanceRunMessage() {}
    InstanceRunMessage(RunType type, Instance *instance) : type(type), instance(instance) {}
    RunType get_type() {
        return type;
    }
    Instance* get_instance() {
        return instance;
    }
private:
    RunType type;
    Instance *instance;
};

// A handler to schedule plugin instance 
class InstanceRunHandler {
public:
    InstanceRunHandler() : cycle(DEFAULT_CYCLE_SIZE) {}
    void run();
    void schedule_collector(uint64_t time);
    void schedule_scenario(uint64_t time);
    void schedule_tune(uint64_t time);
    void handle_instance();
    void set_memory_store(MemoryStore *memory_store) {
        this->memory_store = memory_store;
    }
    void set_cycle(int cycle) {
        this->cycle = cycle;
    }
    int get_cycle() {
        return cycle;
    }
    bool is_instance_exist(const std::string &name) {
        return memory_store->is_instance_exist(name);
    }
    void recv_queue_push(InstanceRunMessage &msg) {
        this->recv_queue.push(msg);
    }
    void recv_queue_push(InstanceRunMessage &&msg) {
        this->recv_queue.push(msg);
    }
    bool recv_queue_try_pop(InstanceRunMessage &msg) {
        return this->recv_queue.try_pop(msg);
    }
private:
    void run_aware(Instance *instance, std::vector<std::string> &deps);
    void run_tune(Instance *instance, std::vector<std::string> &deps);
    void delete_instance(Instance *instance);
    void insert_instance(Instance *instance);
    void adjust_collector_queue(const std::vector<std::string> &deps, const std::vector<std::string> &m_deps, bool flag);
    void check_scenario_dependency(const std::vector<std::string> &deps, const std::vector<std::string> &m_deps);

    std::unordered_map<std::string, Instance*> collector;
    std::unordered_map<std::string, Instance*> scenario;
    std::unordered_map<std::string, Instance*> tune;
    SafeQueue<InstanceRunMessage> recv_queue;
    MemoryStore *memory_store;
    int cycle;
    static const int DEFAULT_CYCLE_SIZE = 10;
    static const int MAX_DEPENDENCIES_SIZE = 20;
};

#endif // !PLUGIN_MGR_INSTANCE_RUN_HANDLER_H

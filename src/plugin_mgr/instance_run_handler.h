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
#include <queue>

enum class RunType {
    ENABLED,
    DISABLED,
    INTERNAL_ENABLED,
    INTERNAL_DISABLED,
 };

/* Message for communication between plugin manager and instance scheduling */
class InstanceRunMessage {
public:
    InstanceRunMessage() {}
    InstanceRunMessage(RunType type, std::shared_ptr<Instance> instance) : type(type), instance(instance), finish(false) { }
    RunType get_type() {
        return type;
    }
    std::shared_ptr<Instance> get_instance() {
        return instance;
    }
    void wait() {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this]() {
            return finish;
        });
    }
    void notify_one() {
        std::unique_lock<std::mutex> lock(mutex);
        finish = true;
        cond.notify_one();
    }
private:
    RunType type;
    std::shared_ptr<Instance> instance;
    std::mutex mutex;
    std::condition_variable cond;
    bool finish;
};

class ScheduleInstance {
public:
    bool operator < (const ScheduleInstance &rhs) const {
        return time > rhs.time || (time == rhs.time && instance->get_priority() > rhs.instance->get_priority());
    }
    std::shared_ptr<Instance> instance;
    uint64_t time;
};

/* A handler to schedule instances. */ 
class InstanceRunHandler {
public:
    using InstanceRun = void (*)(const struct Param*);
    explicit InstanceRunHandler(MemoryStore &memory_store) : memory_store(memory_store), time(0), cycle(DEFAULT_CYCLE_SIZE) { }
    void run();
    void schedule();
    void handle_instance();
    void set_cycle(int cycle) {
        this->cycle = cycle;
    }
    int get_cycle() {
        return cycle;
    }
    void recv_queue_push(std::shared_ptr<InstanceRunMessage> msg) {
        this->recv_queue.push(msg);
    }
    bool recv_queue_try_pop(std::shared_ptr<InstanceRunMessage> &msg) {
        return this->recv_queue.try_pop(msg);
    }
    void add_time(uint64_t period) {
        time += period;
    }
private:
    void run_instance(std::vector<std::string> &deps, InstanceRun run);
    void change_instance_state(const std::string &name, std::vector<std::string> &deps, std::vector<std::string> &after_deps);
    void enable_instance(const std::string &name);
    void disable_instance(const std::string &name, RunType type);
private:
    /* Instance execution queue. */
    std::priority_queue<ScheduleInstance> schedule_queue;
    /* Receives messages from the PluginManager. */
    SafeQueue<std::shared_ptr<InstanceRunMessage>> recv_queue;
    std::unordered_map<std::string, int> in_degree;
    MemoryStore &memory_store;
    uint64_t time;
    int cycle;
    static const int DEFAULT_CYCLE_SIZE = 10;
};

#endif // !PLUGIN_MGR_INSTANCE_RUN_HANDLER_H

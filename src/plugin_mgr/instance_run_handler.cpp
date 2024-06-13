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
#include "instance_run_handler.h"
#include <thread>
#include <unistd.h>

static const DataRingBuf* get_ring_buf(std::shared_ptr<Instance> instance) {
    if (instance == nullptr) {
        return nullptr;
    }
    return instance->get_interface()->get_ring_buf();
}

void InstanceRunHandler::run_instance(std::vector<std::string> &deps, InstanceRun run) {
    std::vector<const DataRingBuf*> input_data;
    for (size_t i = 0; i < deps.size(); ++i) {
        std::shared_ptr<Instance> ins = memory_store.get_instance(deps[i]);
        input_data.emplace_back(get_ring_buf(ins));
    }
    Param param;
    param.ring_bufs = input_data.data();
    param.len = input_data.size();
    run(&param);
}

void InstanceRunHandler::enable_instance(const std::string &name) {
    std::queue<std::shared_ptr<Node>> instance_node_queue;
    auto dep_handler = memory_store.get_dep_handler();
    instance_node_queue.push(dep_handler.get_node(name));
    std::vector<std::string> new_enabled;
    bool enabled = true;
    while (!instance_node_queue.empty()) {
        auto node = instance_node_queue.front();
        instance_node_queue.pop();
        if (node->instance->get_enabled()) {
            continue;
        }
        auto cur_name = node->instance->get_name();
        node->instance->set_enabled(true);
        new_enabled.emplace_back(cur_name);
        if (!node->instance->get_interface()->enable()) {
            enabled = false;
            break;
        }
        for (auto arc = node->head->next; arc != nullptr; arc = arc->next) {
            if (!arc->is_exist) {
                continue;
            }
            auto cur_node = dep_handler.get_node(arc->to);
            in_degree[arc->to]++;
            instance_node_queue.push(cur_node);
        }
    }
    if (!enabled) {
        for (auto &enabled_name : new_enabled) {
            auto instance = dep_handler.get_instance(enabled_name);
            instance->get_interface()->disable();
            instance->set_enabled(false);
            if (enabled_name == name) {
                continue;
            }
            in_degree[enabled_name]--;
        }
    } else {
        for (auto &enabled_name : new_enabled) {
            auto instance = dep_handler.get_instance(enabled_name);
            schedule_queue.push(ScheduleInstance{instance, time});
            INFO("[InstanceRunHandler] " << enabled_name << " instance insert into schedule queue at time " << time);
        }
    }
}

void InstanceRunHandler::disable_instance(const std::string &name, RunType type) {
    if (type == RunType::INTERNAL_DISABLED && in_degree[name] != 0) {
        return;
    }
    in_degree[name] = 0;
    std::queue<std::shared_ptr<Node>> instance_node_queue;
    auto dep_handler = memory_store.get_dep_handler();
    instance_node_queue.push(dep_handler.get_node(name));
    while (!instance_node_queue.empty()) {
        auto node = instance_node_queue.front();
        auto &instance = node->instance;
        instance_node_queue.pop();
        auto cur_name = instance->get_name();
        instance->set_enabled(false);
        instance->get_interface()->disable();
        INFO("[InstanceRunHandler] " << cur_name << " instance disabled at time " << time);
        for (auto arc = node->head->next; arc != nullptr; arc = arc->next) {
            auto cur_node = dep_handler.get_node(arc->to);
            arc->is_exist = arc->init;
            /* The instance can be closed only when the indegree is 0.  */
            if (in_degree[arc->to] <= 0 || --in_degree[arc->to] != 0) {
                continue;
            }
            instance_node_queue.push(cur_node);
        }
    }
}

void InstanceRunHandler::handle_instance() {
    std::shared_ptr<InstanceRunMessage> msg;
    while(this->recv_queue_try_pop(msg)){
        std::shared_ptr<Instance> instance = msg->get_instance();
        switch (msg->get_type()){
            case RunType::ENABLED:
            case RunType::INTERNAL_ENABLED: {
                enable_instance(instance->get_name());
                break;
            }
            case RunType::DISABLED: 
            case RunType::INTERNAL_DISABLED: {
                disable_instance(instance->get_name(), msg->get_type());
                break;
            }
        }
        msg->notify_one();
    }
}

void InstanceRunHandler::change_instance_state(const std::string &name, std::vector<std::string> &deps, 
                                               std::vector<std::string> &after_deps) {
    for (auto &dep : deps) {
        if (std::find(after_deps.begin(), after_deps.end(), dep) != after_deps.end()) {
            continue;
        }
        auto instance = memory_store.get_instance(dep);
        if (instance == nullptr) {
            ERROR("[InstanceRunHandler] ilegal dependency: " << dep);
            continue;
        }
        /* Disable the instance that is not required.  */
        if (instance->get_enabled()) {
            memory_store.delete_edge(name, instance->get_name());
            in_degree[instance->get_name()]--;
            auto msg = std::make_shared<InstanceRunMessage>(RunType::INTERNAL_DISABLED, instance);
            recv_queue_push(std::move(msg));
        }  
    }
    for (auto &after_dep : after_deps) {
        if (std::find(deps.begin(), deps.end(), after_dep) != deps.end()) {
            continue;
        }
        auto instance = memory_store.get_instance(after_dep);
        if (instance == nullptr) {
            ERROR("[InstanceRunHandler] ilegal dependency: " << after_dep);
            continue;
        }
        /* Enable the instance that is required. */
        if (!instance->get_enabled()) {
            in_degree[instance->get_name()]++;
            memory_store.add_edge(name, instance->get_name());
            auto msg = std::make_shared<InstanceRunMessage>(RunType::INTERNAL_ENABLED, instance);
            recv_queue_push(std::move(msg));
        }
    }    
}

void InstanceRunHandler::schedule() {
    while (!schedule_queue.empty()) {
        auto schedule_instance = schedule_queue.top();
        auto &instance = schedule_instance.instance;
        if (schedule_instance.time != time) {
            break;
        }
        schedule_queue.pop();
        if (!instance->get_enabled()) {
            continue;
        }
        std::vector<std::string> deps = instance->get_deps();
        run_instance(deps, instance->get_interface()->run);
        schedule_instance.time += instance->get_interface()->get_period();
        schedule_queue.push(schedule_instance);
        /* Dynamically change dependencies. */
        std::vector<std::string> after_deps = instance->get_deps();
        change_instance_state(instance->get_name(), deps, after_deps);
    }
}

void start(InstanceRunHandler *instance_run_handler) {
    INFO("[PluginManager] instance schedule started!");
    while(true) {
        instance_run_handler->handle_instance();
        instance_run_handler->schedule();
        usleep(instance_run_handler->get_cycle() * 1000);
        instance_run_handler->add_time(instance_run_handler->get_cycle());
    }
}

void InstanceRunHandler::run() {
    std::thread t(start, this);
    t.detach();
}

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

namespace oeaware {

void InstanceRunHandler::RunInstance(std::vector<std::string> &deps, InstanceRun run)
{
    std::vector<const DataRingBuf*> input_data;
    for (size_t i = 0; i < deps.size(); ++i) {
        std::shared_ptr<Instance> ins = memoryStore->GetInstance(deps[i]);
        if (ins == nullptr) {
            continue;
        }
        auto buf = ins->GetInterface()->get_ring_buf();
        input_data.emplace_back(buf);
    }
    Param param;
    param.ring_bufs = input_data.data();
    param.len = input_data.size();
    run(&param);
}

void InstanceRunHandler::EnableInstance(const std::string &name)
{
    std::queue<std::shared_ptr<Node>> instance_node_queue;
    auto dep_handler = memoryStore->GetDepHandler();
    instance_node_queue.push(dep_handler.GetNode(name));
    std::vector<std::string> new_enabled;
    bool enabled = true;
    while (!instance_node_queue.empty()) {
        auto node = instance_node_queue.front();
        instance_node_queue.pop();
        if (node->instance->GetEnabled()) {
            continue;
        }
        auto cur_name = node->instance->GetName();
        node->instance->SetEnabled(true);
        new_enabled.emplace_back(cur_name);
        if (!node->instance->GetInterface()->enable()) {
            enabled = false;
            break;
        }
        for (auto arc = node->head->next; arc != nullptr; arc = arc->next) {
            if (!arc->isExist) {
                continue;
            }
            auto cur_node = dep_handler.GetNode(arc->to);
            inDegree[arc->to]++;
            instance_node_queue.push(cur_node);
        }
    }
    if (!enabled) {
        for (auto &enabled_name : new_enabled) {
            auto instance = dep_handler.GetInstance(enabled_name);
            instance->GetInterface()->disable();
            instance->SetEnabled(false);
            if (enabled_name == name) {
                continue;
            }
            inDegree[enabled_name]--;
        }
    } else {
        for (auto &enabled_name : new_enabled) {
            auto instance = dep_handler.GetInstance(enabled_name);
            scheduleQueue.push(ScheduleInstance{instance, time});
            INFO(logger, enabled_name << " instance insert into schedule queue at time " << time);
        }
    }
}

void InstanceRunHandler::DisableInstance(const std::string &name, bool force)
{
    if (!force && inDegree[name] != 0) {
        return;
    }
    inDegree[name] = 0;
    std::queue<std::shared_ptr<Node>> instance_node_queue;
    auto dep_handler = memoryStore->GetDepHandler();
    instance_node_queue.push(dep_handler.GetNode(name));
    while (!instance_node_queue.empty()) {
        auto node = instance_node_queue.front();
        auto &instance = node->instance;
        instance_node_queue.pop();
        auto cur_name = instance->GetName();
        instance->SetEnabled(false);
        instance->GetInterface()->disable();
        INFO(logger, cur_name << " instance disabled at time " << time);
        for (auto arc = node->head->next; arc != nullptr; arc = arc->next) {
            auto cur_node = dep_handler.GetNode(arc->to);
            arc->isExist = arc->init;
            /* The instance can be closed only when the indegree is 0.  */
            if (inDegree[arc->to] <= 0 || --inDegree[arc->to] != 0) {
                continue;
            }
            instance_node_queue.push(cur_node);
        }
    }
}

bool InstanceRunHandler::HandleMessage()
{
    std::shared_ptr<InstanceRunMessage> msg;
    bool shutdown = false;
    while (this->RecvQueueTryPop(msg)) {
        std::shared_ptr<Instance> instance = msg->GetInstance();
        switch (msg->GetType()) {
            case RunType::ENABLED: {
                EnableInstance(instance->GetName());
                break;
            }
            case RunType::DISABLED: {
                DisableInstance(instance->GetName(), true);
                break;
            }
            case RunType::SHUTDOWN: {
                shutdown = true;
                break;
            }
        }
        msg->NotifyOne();
        if (shutdown) {
            return false;
        }
    }
    return true;
}

void InstanceRunHandler::ChangeInstanceState(const std::string &name, std::vector<std::string> &deps,
    std::vector<std::string> &after_deps)
{
    for (auto &dep : deps) {
        if (std::find(after_deps.begin(), after_deps.end(), dep) != after_deps.end()) {
            continue;
        }
        auto instance = memoryStore->GetInstance(dep);
        if (instance == nullptr) {
            ERROR(logger, "ilegal dependency: " << dep);
            continue;
        }
        memoryStore->DeleteEdge(name, instance->GetName());
        inDegree[instance->GetName()]--;
        /* Disable the instance that is not required.  */
        if (instance->GetEnabled()) {
            DisableInstance(instance->GetName(), false);
        }
    }
    for (auto &after_dep : after_deps) {
        if (std::find(deps.begin(), deps.end(), after_dep) != deps.end()) {
            continue;
        }
        auto instance = memoryStore->GetInstance(after_dep);
        if (instance == nullptr) {
            ERROR(logger, "ilegal dependency: " << after_dep);
            continue;
        }
        inDegree[instance->GetName()]++;
        memoryStore->AddEdge(name, instance->GetName());
        /* Enable the instance that is required. */
        if (!instance->GetEnabled()) {
            EnableInstance(instance->GetName());
        }
    }
}

void InstanceRunHandler::Schedule()
{
    while (!scheduleQueue.empty()) {
        auto schedule_instance = scheduleQueue.top();
        auto &instance = schedule_instance.instance;
        if (schedule_instance.time != time) {
            break;
        }
        scheduleQueue.pop();
        if (!instance->GetEnabled()) {
            continue;
        }
        std::vector<std::string> deps = instance->GetDeps();
        RunInstance(deps, instance->GetInterface()->run);
        schedule_instance.time += instance->GetInterface()->get_period();
        scheduleQueue.push(schedule_instance);
        /* Dynamically change dependencies. */
        std::vector<std::string> after_deps = instance->GetDeps();
        ChangeInstanceState(instance->GetName(), deps, after_deps);
    }
}

void InstanceRunHandler::Start()
{
    INFO(logger, "instance schedule started!");
    const static uint64_t millisecond = 1000;
    while (true) {
        if (!HandleMessage()) {
            INFO(logger, "instance schedule shutdown!");
            break;
        }
        Schedule();
        usleep(GetCycle() * millisecond);
        AddTime(GetCycle());
    }
}

void InstanceRunHandler::Init()
{
    Logger::GetInstance().Register("InstanceSchedule");
    logger = Logger::GetInstance().Get("InstanceSchedule");
}

void InstanceRunHandler::Run()
{
    Init();
    std::thread t([this] {
        this->Start();
    });
    t.detach();
}
}

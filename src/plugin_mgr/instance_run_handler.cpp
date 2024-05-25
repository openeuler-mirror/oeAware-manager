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

static const void* get_ring_buf(std::shared_ptr<Instance> instance) {
    if (instance == nullptr) {
        return nullptr;
    }
    return instance->get_interface()->get_ring_buf();
}

void InstanceRunHandler::run_instance(std::shared_ptr<Instance> instance) {
    std::vector<const void*> input_data;
    std::vector<std::string> deps = instance->get_deps();
    for (size_t i = 0; i < deps.size(); ++i) {
        std::shared_ptr<Instance> ins = memory_store.get_instance(deps[i]);
        input_data.emplace_back(get_ring_buf(ins));
    }
    Param param;
    param.args = input_data.data();
    param.len = input_data.size();
    instance->get_interface()->run(&param);
}

void InstanceRunHandler::insert_instance(std::shared_ptr<Instance> instance, uint64_t time) {
    instance->set_enabled(true);
    schedule_queue.push(ScheduleInstance{instance, time});
    INFO("[PluginManager] " << instance->get_name() << " instance insert into running queue.");
}

void InstanceRunHandler::delete_instance(std::shared_ptr<Instance> instance) {
    instance->get_interface()->disable();
    instance->set_enabled(false);
    INFO("[PluginManager] " << instance->get_name() << " instance delete from running queue.");
}

void InstanceRunHandler::handle_instance(uint64_t time) {
    InstanceRunMessage msg;
    while(this->recv_queue_try_pop(msg)){
        std::shared_ptr<Instance> instance = msg.get_instance();
        switch (msg.get_type()){
            case RunType::ENABLED: {
                insert_instance(std::move(instance), time);
                break;
            }
            case RunType::DISABLED: {
                delete_instance(std::move(instance));
                break;
            }
        }
    }
}

void InstanceRunHandler::schedule(uint64_t time) {
    while (!schedule_queue.empty()) {
        auto schedule_instance = schedule_queue.top();
        if (schedule_instance.time != time) {
            break;
        }
        schedule_queue.pop();
        if (!schedule_instance.instance->get_enabled()) {
            break;
        }
        run_instance(schedule_instance.instance);
        schedule_instance.time += schedule_instance.instance->get_interface()->get_cycle();
        schedule_queue.push(schedule_instance);
    }
}

void start(InstanceRunHandler *instance_run_handler) {
    unsigned long long time = 0;
    INFO("[PluginManager] instance schedule started!");
    while(true) {
        instance_run_handler->handle_instance(time);
        instance_run_handler->schedule(time);
        usleep(instance_run_handler->get_cycle() * 1000);
        time += instance_run_handler->get_cycle();
    }
}

void InstanceRunHandler::run() {
    std::thread t(start, this);
    t.detach();
}

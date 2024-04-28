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

static void* get_ring_buf(std::shared_ptr<Instance> instance) {
    if (instance == nullptr) {
        return nullptr;
    }
    switch (instance->get_type()) {
        case PluginType::COLLECTOR:
            return (std::dynamic_pointer_cast<CollectorInstance>(instance))->get_interface()->get_ring_buf();
        case PluginType::SCENARIO:
            return (std::dynamic_pointer_cast<ScenarioInstance>(instance))->get_interface()->get_ring_buf();
        case PluginType::TUNE:
            break;
    }
    return nullptr;
}

static void reflash_ring_buf(std::shared_ptr<Instance> instance) {
    (std::dynamic_pointer_cast<CollectorInstance>(instance))->get_interface()->reflash_ring_buf();
}

void InstanceRunHandler::run_aware(std::shared_ptr<Instance> instance, std::vector<std::string> &deps) {
    void *a[MAX_DEPENDENCIES_SIZE];
    for (int i = 0; i < deps.size(); ++i) {
        std::shared_ptr<Instance> ins = memory_store.get_instance(deps[i]);
        a[i] = get_ring_buf(ins);
    }
    (std::dynamic_pointer_cast<ScenarioInstance>(instance))->get_interface()->aware(a, (int)deps.size());
}

void InstanceRunHandler::run_tune(std::shared_ptr<Instance> instance, std::vector<std::string> &deps) {
    void *a[MAX_DEPENDENCIES_SIZE];
    for (int i = 0; i < deps.size(); ++i) {
        std::shared_ptr<Instance> ins = memory_store.get_instance(deps[i]);
        a[i] = get_ring_buf(ins);
    }
    (std::dynamic_pointer_cast<TuneInstance>(instance))->get_interface()->tune(a, (int)deps.size());
}

void InstanceRunHandler::insert_instance(std::shared_ptr<Instance> instance) {
    switch (instance->get_type()) {
        case PluginType::COLLECTOR:
            collector[instance->get_name()] = instance;
            (std::dynamic_pointer_cast<CollectorInstance>(instance))->get_interface()->enable();
            break;
        case PluginType::SCENARIO:
            scenario[instance->get_name()] = instance;
            (std::dynamic_pointer_cast<ScenarioInstance>(instance))->get_interface()->enable();
            break;
        case PluginType::TUNE:
            tune[instance->get_name()] = instance;
            (std::dynamic_pointer_cast<TuneInstance>(instance))->get_interface()->enable();
            break;
    }
    INFO("[PluginManager] " << instance->get_name() << " instance insert into running queue.");
}

void InstanceRunHandler::delete_instance(std::shared_ptr<Instance> instance) {
    switch (instance->get_type()) {
        case PluginType::COLLECTOR:
            collector.erase(instance->get_name());
            (std::dynamic_pointer_cast<CollectorInstance>(instance))->get_interface()->disable();
            break;
        case PluginType::SCENARIO:
            scenario.erase(instance->get_name());
            (std::dynamic_pointer_cast<ScenarioInstance>(instance))->get_interface()->disable();
            break;
        case PluginType::TUNE:
            tune.erase(instance->get_name());
            (std::dynamic_pointer_cast<TuneInstance>(instance))->get_interface()->disable();
            break;
    }
    INFO("[PluginManager] " << instance->get_name() << " instance delete from running queue.");
}

void InstanceRunHandler::handle_instance() {
    InstanceRunMessage msg;
    while(this->recv_queue_try_pop(msg)){
        std::shared_ptr<Instance> instance = msg.get_instance();
        switch (msg.get_type()){
            case RunType::ENABLED:
                insert_instance(std::move(instance));
                break;
            case RunType::DISABLED:
                delete_instance(std::move(instance));
                break;
        }
    }
}

template<typename T>
static std::vector<std::string> get_deps(std::shared_ptr<Instance> instance) {
    std::shared_ptr<T> t_instance = std::dynamic_pointer_cast<T>(instance);
    std::string deps = (t_instance)->get_interface()->get_dep();
    std::string dep = "";
    std::vector<std::string> vec;
    for (int i = 0; i < deps.length(); ++i) {
        if (deps[i] != '-') {
            dep += deps[i];
        }else {
            vec.emplace_back(dep);
            dep = "";
        }
    }
    vec.emplace_back(dep);
    return vec;
}

void InstanceRunHandler::adjust_collector_queue(const std::vector<std::string> &deps, const std::vector<std::string> &m_deps, bool flag) {
    for (auto &m_dep : m_deps) {
        bool ok = false;
        for (auto &dep : deps) {
            if (m_dep == dep) {
                ok = true;
            }
        }
        if (ok) continue;
        if (flag) {
            if (is_instance_exist(m_dep) && !collector.count(m_dep)) {
                this->insert_instance(memory_store.get_instance(m_dep));
            }
        } else {
            if (is_instance_exist(m_dep) && collector.count(m_dep)) {
                this->delete_instance(memory_store.get_instance(m_dep));
            }
        }
    }
}

void InstanceRunHandler::check_scenario_dependency(const std::vector<std::string> &origin_deps, const std::vector<std::string> &cur_deps) {
    adjust_collector_queue(origin_deps, cur_deps, true);
    adjust_collector_queue(cur_deps, origin_deps, false);
}

void InstanceRunHandler::schedule_collector(uint64_t time) {
    for (auto &p : collector) {
        std::shared_ptr<Instance> instance = p.second;
        int t = (std::dynamic_pointer_cast<CollectorInstance>(instance))->get_interface()->get_cycle();
        if (time % t != 0) return;
        reflash_ring_buf(instance);
    }
}

void InstanceRunHandler::schedule_scenario(uint64_t time) {
    for (auto &p : scenario) {
        std::shared_ptr<Instance> instance = p.second;
        int t = (std::dynamic_pointer_cast<ScenarioInstance>(instance))->get_interface()->get_cycle();
        if (time % t != 0) return;
        std::vector<std::string> origin_deps = get_deps<ScenarioInstance>(instance); 
        run_aware(instance, origin_deps); 
        std::vector<std::string> cur_deps = get_deps<ScenarioInstance>(instance);
        check_scenario_dependency(origin_deps, cur_deps);
    }
}

void InstanceRunHandler::schedule_tune(uint64_t time) {
    for (auto &p : tune) {
        std::shared_ptr<Instance> instance = p.second;
        int t = (std::dynamic_pointer_cast<TuneInstance>(instance))->get_interface()->get_cycle();
        if (time % t != 0) return;
        std::vector<std::string> deps = get_deps<TuneInstance>(instance); 
        run_tune(instance, deps);
    }
}

void start(InstanceRunHandler *instance_run_handler) {
    unsigned long long time = 0;
    INFO("[PluginManager] instance schedule started!");
    while(true) {
        instance_run_handler->handle_instance();
        instance_run_handler->schedule_collector(time);
        instance_run_handler->schedule_scenario(time);
        instance_run_handler->schedule_tune(time);

        usleep(instance_run_handler->get_cycle() * 1000);
        time += instance_run_handler->get_cycle();
    }
}

void InstanceRunHandler::run() {
    std::thread t(start, this);
    t.detach();
}

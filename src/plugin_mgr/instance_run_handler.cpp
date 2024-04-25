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

static void* get_ring_buf(Instance *instance) {
    if (instance == nullptr) {
        return nullptr;
    }
    switch (instance->get_type()) {
        case PluginType::COLLECTOR:
            return ((CollectorInstance*)instance)->get_interface()->get_ring_buf();
        case PluginType::SCENARIO:
            return ((ScenarioInstance*)instance)->get_interface()->get_ring_buf();
        case PluginType::TUNE:
            break;
    }
    return nullptr;
}

static void reflash_ring_buf(Instance *instance) {
    ((CollectorInstance*)instance)->get_interface()->reflash_ring_buf();
}

void InstanceRunHandler::run_aware(Instance *instance, std::vector<std::string> &deps) {
    void *a[MAX_DEPENDENCIES_SIZE];
    for (int i = 0; i < deps.size(); ++i) {
        Instance *ins = memory_store->get_instance(deps[i]);
        a[i] = get_ring_buf(ins);
    }
    ((ScenarioInstance*)instance)->get_interface()->aware(a, (int)deps.size());
}

void InstanceRunHandler::run_tune(Instance *instance, std::vector<std::string> &deps) {
    void *a[MAX_DEPENDENCIES_SIZE];
    for (int i = 0; i < deps.size(); ++i) {
        Instance *ins = memory_store->get_instance(deps[i]);
        a[i] = get_ring_buf(ins);
    }
    ((TuneInstance*)instance)->get_interface()->tune(a, (int)deps.size());
}

void InstanceRunHandler::insert_instance(Instance *instance) {
    switch (instance->get_type()) {
        case PluginType::COLLECTOR:
            collector[instance->get_name()] = instance;
            ((CollectorInstance*)instance)->get_interface()->enable();
            break;
        case PluginType::SCENARIO:
            scenario[instance->get_name()] = instance;
            ((ScenarioInstance*)instance)->get_interface()->enable();
            break;
        case PluginType::TUNE:
            tune[instance->get_name()] = instance;
            ((TuneInstance*)instance)->get_interface()->enable();
            break;
    }
    INFO("[PluginManager] " << instance->get_name() << " instance insert into running queue.");
}

void InstanceRunHandler::delete_instance(Instance *instance) {
    switch (instance->get_type()) {
        case PluginType::COLLECTOR:
            collector.erase(instance->get_name());
            ((CollectorInstance*)instance)->get_interface()->disable();
            break;
        case PluginType::SCENARIO:
            scenario.erase(instance->get_name());
            ((ScenarioInstance*)instance)->get_interface()->disable();
            break;
        case PluginType::TUNE:
            tune.erase(instance->get_name());
            ((TuneInstance*)instance)->get_interface()->disable();
            break;
    }
    INFO("[PluginManager] " << instance->get_name() << " instance delete from running queue.");
}

void InstanceRunHandler::handle_instance() {
    InstanceRunMessage msg;
    while(this->recv_queue_try_pop(msg)){
        Instance *instance = msg.get_instance();
        switch (msg.get_type()){
            case RunType::ENABLED:
                insert_instance(instance);
                break;
            case RunType::DISABLED:
                delete_instance(instance);
                break;
        }
    }
}

template<typename T>
static std::vector<std::string> get_deps(Instance *instance) {
    std::string deps = ((T*)instance)->get_interface()->get_dep();
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
                this->insert_instance(memory_store->get_instance(m_dep));
            }
        } else {
            if (is_instance_exist(m_dep) && collector.count(m_dep)) {
                this->delete_instance(memory_store->get_instance(m_dep));
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
        Instance *instance = p.second;
        int t = ((CollectorInstance*)instance)->get_interface()->get_cycle();
        if (time % t != 0) return;
        reflash_ring_buf(instance);
    }
}

void InstanceRunHandler::schedule_scenario(uint64_t time) {
    for (auto &p : scenario) {
        Instance *instance = p.second;
        int t = ((ScenarioInstance*)instance)->get_interface()->get_cycle();
        if (time % t != 0) return;
        std::vector<std::string> origin_deps = get_deps<ScenarioInstance>(instance); 
        run_aware(instance, origin_deps); 
        std::vector<std::string> cur_deps = get_deps<ScenarioInstance>(instance);
        check_scenario_dependency(origin_deps, cur_deps);
    }
}

void InstanceRunHandler::schedule_tune(uint64_t time) {
    for (auto &p : tune) {
        Instance *instance = p.second;
        int t = ((TuneInstance*)instance)->get_interface()->get_cycle();
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

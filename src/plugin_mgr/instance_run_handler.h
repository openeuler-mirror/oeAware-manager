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
#include <queue>
#include "safe_queue.h"
#include "plugin.h"
#include "logger.h"
#include "memory_store.h"

namespace oeaware {
enum class RunType {
    /* Message from PluginManager. */
    ENABLED,
    DISABLED,
    SHUTDOWN,
 };

/* Message for communication between plugin manager and instance scheduling */
class InstanceRunMessage {
public:
    InstanceRunMessage() {}
    InstanceRunMessage(RunType type, std::shared_ptr<Instance> instance) : type(type), instance(instance),
        finish(false) { }
    RunType GetType()
    {
        return type;
    }
    std::shared_ptr<Instance> GetInstance()
    {
        return instance;
    }
    void Wait()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this]() {
            return finish;
        });
    }
    void NotifyOne()
    {
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
    bool operator < (const ScheduleInstance &rhs) const
    {
        return time > rhs.time || (time == rhs.time &&
            instance->interface->GetPriority() > rhs.instance->interface->GetPriority());
    }
    std::shared_ptr<Instance> instance;
    uint64_t time;
};

/* A handler to schedule instances. */
class InstanceRunHandler {
public:
    using InstanceRun = void (*)();
    explicit InstanceRunHandler(std::shared_ptr<MemoryStore> memoryStore) : memoryStore(memoryStore), time(0),
        cycle(defaultCycleSize) { }
    void Init();
    void Run();
    void Schedule();
    bool HandleMessage();
    void SetCycle(uint64_t newCycle)
    {
        this->cycle = newCycle;
    }
    int GetCycle()
    {
        return cycle;
    }
    void RecvQueuePush(std::shared_ptr<InstanceRunMessage> msg)
    {
        this->RecvQueue.Push(msg);
    }
    bool RecvQueueTryPop(std::shared_ptr<InstanceRunMessage> &msg)
    {
        return this->RecvQueue.TryPop(msg);
    }
    void AddTime(uint64_t period)
    {
        time += period;
    }
private:
    void Start();
    void EnableInstance(const std::string &name);
    void DisableInstance(const std::string &name, bool force);
private:
    /* Instance execution queue. */
    std::priority_queue<ScheduleInstance> scheduleQueue;
    /* Receives messages from the PluginManager. */
    SafeQueue<std::shared_ptr<InstanceRunMessage>> RecvQueue;
    std::unordered_map<std::string, int> inDegree;
    std::shared_ptr<MemoryStore> memoryStore;
    log4cplus::Logger logger;
    uint64_t time;
    uint64_t cycle;
    static const uint64_t defaultCycleSize = 10;
};
}

#endif // !PLUGIN_MGR_INSTANCE_RUN_HANDLER_H

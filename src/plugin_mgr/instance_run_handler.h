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
#include <unordered_set>
#include "event.h"
#include "safe_queue.h"
#include "logger.h"
#include "memory_store.h"
#include "data_register.h"
#include "instance_run_message.h"

namespace oeaware {
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

using TopicState = std::unordered_map<std::string, std::unordered_map<std::string,
    std::unordered_map<std::string, bool>>>;

/* A handler to schedule instances. */
class InstanceRunHandler {
public:
    using InstanceRun = void (*)();
    InstanceRunHandler(std::shared_ptr<MemoryStore> memoryStore, EventQueue recvData,
        std::shared_ptr<SafeQueue<std::shared_ptr<InstanceRunMessage>>> recvQueue) : memoryStore(memoryStore),
        recvData(recvData), recvQueue(recvQueue), time(0), cycle(defaultCycleSize) { }
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
        recvQueue->Push(msg);
    }
    bool RecvQueueTryPop(std::shared_ptr<InstanceRunMessage> &msg)
    {
        return recvQueue->TryPop(msg);
    }
    void AddTime(uint64_t period)
    {
        time += period;
    }
    std::unordered_map<std::string, std::unordered_set<std::string>> GetSubscribers() const
    {
        return subscibers;
    }
private:
    void Start();
    void UpdateData();
    Result Subscribe(const std::vector<std::string> &payload);
    Result Unsubscribe(const std::vector<std::string> &payload);
    Result UnsubscribeSdk(const std::vector<std::string> &payload);
    void UpdateInstance();
    Result EnableInstance(const std::string &name);
    void DisableInstance(const std::string &name);
    Result Publish(const std::vector<std::string> &payload);
    void CloseInstance(std::shared_ptr<Instance> instance);
    void PublishData(std::shared_ptr<InstanceRunMessage> &msg);
    /* Instance execution queue. */
    std::priority_queue<ScheduleInstance> scheduleQueue;
    /* Receives messages from the PluginManager. */
    std::shared_ptr<MemoryStore> memoryStore;
    EventQueue recvData;
    std::shared_ptr<SafeQueue<std::shared_ptr<InstanceRunMessage>>> recvQueue;
    std::vector<std::pair<Topic, std::string>> topicRunOnce;
    TopicState topicState;
    std::unordered_map<std::string, std::unordered_set<std::string>> subscibers;
    log4cplus::Logger logger;
    uint64_t time;
    uint64_t cycle;
    static const uint64_t defaultCycleSize = 10;
};

using InstanceRunHandlerPtr = std::shared_ptr<InstanceRunHandler>;

}

#endif // !PLUGIN_MGR_INSTANCE_RUN_HANDLER_H

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
#ifndef OEAWARE_INTERFACE_H
#define OEAWARE_INTERFACE_H
#include <oeaware/data_list.h>
#include <oeaware/logger.h>
#include <oeaware/safe_queue.h>
#include <oeaware/instance_run_message.h>

namespace oeaware {
// Instance type.
const int TUNE = 0b10000;
const int SCENARIO = 0b01000;
const int RUN_ONCE = 0b00010;

class Interface {
public:
    Interface() = default;
    virtual ~Interface() = default;
    void SetRecvQueue(std::shared_ptr<SafeQueue<std::shared_ptr<InstanceRunMessage>>> newRecvQueue)
    {
        recvQueue = newRecvQueue;
    }
    void SetLogger(const log4cplus::Logger &newLogger)
    {
        logger = newLogger;
    }
    std::string GetName() const
    {
        return name;
    }
    std::string GetVersion() const
    {
        return version;
    }
    std::string GetDescription() const
    {
        return description;
    }
    int GetType() const
    {
        return type;
    }
    int GetPriority() const
    {
        return priority;
    }
    int GetPeriod() const
    {
        return period;
    }
    std::vector<Topic> GetSupportTopics() const
    {
        return supportTopics;
    }
    virtual Result OpenTopic(const Topic &topic) = 0;
    virtual void CloseTopic(const Topic &topic) = 0;
    virtual void UpdateData(const DataList &dataList) = 0;
    virtual Result Enable(const std::string &param = "") = 0;
    virtual void Disable() = 0;
    virtual void Run() = 0;
protected:
    std::string name;
    std::string version;
    std::string description;
    std::vector<Topic> supportTopics;
    log4cplus::Logger logger;
    int priority;
    int type;
    int period;
    Result Subscribe(const Topic &topic)
    {
        auto msg = std::make_shared<InstanceRunMessage>(RunType::SUBSCRIBE,
            std::vector<std::string>{topic.GetType(), name});
        recvQueue->Push(msg);
        return Result(OK);
    }
    Result Unsubscribe(const Topic &topic)
    {
        auto msg = std::make_shared<InstanceRunMessage>(RunType::UNSUBSCRIBE,
            std::vector<std::string>{topic.GetType(), name});
        recvQueue->Push(msg);
        return Result(OK);
    }
    void Publish(DataList &dataList, bool isFree = true)
    {
        Topic topic{dataList.topic.instanceName, dataList.topic.topicName, dataList.topic.params};
        auto msg = std::make_shared<InstanceRunMessage>(RunType::PUBLISH_DATA,
            std::vector<std::string>{topic.GetType()});
        msg->isFree = isFree;
        msg->dataList.data = dataList.data;
        msg->dataList.len = dataList.len;
        msg->dataList.topic.instanceName = dataList.topic.instanceName;
        msg->dataList.topic.topicName = dataList.topic.topicName;
        msg->dataList.topic.params = dataList.topic.params;
        recvQueue->Push(msg);
    }
private:
    std::shared_ptr<SafeQueue<std::shared_ptr<InstanceRunMessage>>> recvQueue;
};
}

#endif

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
#ifndef COMMON_INTERFACE_H
#define COMMON_INTERFACE_H
#include "data_list.h"
#include "manager_callback.h"

namespace oeaware {
class Interface {
public:
    Interface() = default;
    virtual ~Interface() = default;
    int Subscribe(const Topic &topic)
    {
        return managerCallback->Subscribe(name, topic, 1);
    }
    int Unsubscribe(const Topic &topic)
    {
        return managerCallback->Unsubscribe(name, topic, 1);
    }
    void Publish(const DataList &dataList)
    {
        return managerCallback->Publish(dataList);
    }
    void SetManagerCallback(std::shared_ptr<ManagerCallback> newManagerCallback)
    {
        this->managerCallback = newManagerCallback;
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
    virtual int OpenTopic(const Topic &topic) = 0;
    virtual void CloseTopic(const Topic &topic) = 0;
    virtual void UpdateData(const DataList &dataList) = 0;
    virtual int Enable() = 0;
    virtual void Disable() = 0;
    virtual void Run() = 0;
protected:
    std::string name;
    std::string version;
    std::string description;
    std::vector<Topic> supportTopics;
    int priority;
    int type;
    int period;
private:
    std::shared_ptr<ManagerCallback> managerCallback;
};

void GetInstance(std::vector<std::shared_ptr<Interface>> &interface);
}

#endif

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
#include "manager_callback.h"
#include "base_data.h"
namespace oeaware {
class Interface {
public:
    Interface() = default;
    virtual ~Interface() = default;
    void SetCallback(std::shared_ptr<ManagerCallback> callback)
    {
        this->managerCallback = std::move(callback);
    }

    std::string GetName() { return name; }
    std::string GetDescription() { return description; }
    int GetPriority() const { return priority; }
    int GetType() const { return type; }
    int GetPeriod() const { return period; }

    virtual bool OpenTopic(const Topic &topic) = 0;
    virtual void CloseTopic(const Topic &topic) = 0;
    virtual void UpdateData(const DataList &dataList) = 0;
    virtual std::vector<std::string> GetSupportTopics() = 0;
    virtual int Enable() = 0;
    virtual void Disable() = 0;
    virtual void Run() = 0;

protected:
    std::string name;
    std::string version;
    std::string description;
    int priority{};
    int type{};
    int period{};
    int Subscribe(const Topic &topic)
    {
        return managerCallback->Subscribe(topic);
    }
    void Unsubscribe(const Topic &topic)
    {
        return managerCallback->Unsubscribe(topic);
    }
    void Publish(const DataList &dataList)
    {
        return managerCallback->Publish(dataList);
    }
private:
    std::shared_ptr<ManagerCallback> managerCallback;
};

void GetInstance(std::vector<std::shared_ptr<Interface>> &interface);
}

#endif

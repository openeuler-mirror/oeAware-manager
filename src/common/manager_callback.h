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
#ifndef PLUGIN_MGR_MANAGER_CALLBACK_H
#define PLUGIN_MGR_MANAGER_CALLBACK_H
#include <unordered_set>
#include <unordered_map>
#include "data_list.h"
#include "topic.h"
#include "event.h"

namespace oeaware {
using InDegree = std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, int>>>;
class ManagerCallback {
public:
    /**
     * @brief
     * @param name          Subscriber name.
     * @param topic         Subscribed topic.
     * @param type          0 sdk, 1 instance.
     */
    Result Subscribe(const std::string &name, const Topic &topic, int type);
    /**
     * @brief
     * @param name          Unsubscriber name.
     * @param topic         Unsubscribed topic.
     * @param type          0 sdk, 1 instance.
     */
    Result Unsubscribe(const std::string &name, const Topic &topic, int type);
    /**
     * @brief Unsubscribe all topics subscribed by "name".
     * @param name          Unsubscriber name.
     * @param topic         Unsubscribed topic.
     * @param type          0 sdk, 1 instance.
     * @return Instances subscribed by the subscribers.
     */
    std::vector<std::string> Unsubscribe(const std::string &name);
    void Publish(const DataList &dataList);
    void Init(EventQueue newRecvData);
    
    // Data to be updated for the instance.
    std::vector<DataList> publishData;
    std::unordered_map<std::string, std::unordered_set<std::string>> topicInstance;
    std::unordered_map<std::string, std::unordered_set<std::string>> topicSdk;
    InDegree inDegree;
private:
    std::mutex mutex;
    EventQueue recvData;
};
using ManagerCallbackPtr = std::shared_ptr<ManagerCallback>;
}

#endif

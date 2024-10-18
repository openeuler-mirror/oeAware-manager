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
#include <set>
#include <unordered_map>
#include "data_list.h"
#include "event.h"

namespace oeaware {
class ManagerCallback {
public:
    int Subscribe(const std::string &name, const Topic &topic, int type);
    int Unsubscribe(const std::string &name, const Topic &topic, int type);
    void Publish(const DataList &dataList);
    void Init(EventQueue newRecvData);
private:
    std::unordered_map<std::string, std::set<std::pair<std::string, int>>> subscriber;
    std::mutex mutex;
    EventQueue recvData;
};
}

#endif

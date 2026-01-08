/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef SOFT_DOMAIN_TUNE_H
#define SOFT_DOMAIN_TUNE_H

#include "oeaware/interface.h"
#include "oeaware/data/docker_data.h"
#include "oeaware/data/thread_info.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace oeaware {

/**
 * @brief SoftDomainTune类，用于使能SOFT_DOMAIN调度特性
 */
class SoftDomainTune : public Interface {
public:
    SoftDomainTune();
    ~SoftDomainTune() override = default;
    
    // Interface实现
    Result OpenTopic(const Topic &topic) override;
    void CloseTopic(const Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    // 更新docker数据
    void UpdateDockerData(const DataList &dataList);
    // 更新线程数据
    void UpdateThreadData(const DataList &dataList);
    
    const int defaultPeriod = 1000;
    const int defaultPriority = 2;
    
    // 订阅的topic列表
    std::vector<Topic> subscribeTopics;
    
    // 存储docker信息，key为docker id，value为Container信息
    std::unordered_map<std::string, Container> dockerContainers;
    
    // 存储线程信息，key为tid，value为线程名称
    std::unordered_map<int, std::string> threadInfos;
};

} // namespace oeaware

#endif // SOFT_DOMAIN_TUNE_H


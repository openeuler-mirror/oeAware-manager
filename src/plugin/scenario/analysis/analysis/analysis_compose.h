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
#ifndef ANALYSIS_COMPOSE_H
#define ANALYSIS_COMPOSE_H
#include <string>
#include "oeaware/topic.h"
namespace oeaware {
class AnalysisCompose {
public:
    virtual void Init() = 0;
    virtual void UpdateData(const std::string &topicName, void *data) = 0;
    virtual void Analysis() = 0;
    virtual void* GetResult() = 0;
    virtual void Reset() = 0;
    std::vector<Topic> subTopics;
};
}

#endif

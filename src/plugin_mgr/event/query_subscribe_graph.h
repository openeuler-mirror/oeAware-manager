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
#ifndef PLUGIN_MGR_EVENT_QUERY_SUBSCRIBE_GRAPH_H
#define PLUGIN_MGR_EVENT_QUERY_SUBSCRIBE_GRAPH_H
#include "event_handler.h"
#include "instance_run_handler.h"

namespace oeaware {
class QuerySubscribeGraphHandler : public Handler {
public:
    explicit QuerySubscribeGraphHandler(std::shared_ptr<InstanceRunHandler> instanceRunHandler)
        : instanceRunHandler(instanceRunHandler) {}
    EventResult Handle(const Event &event) override;
private:
    ErrorCode QuerySubGraph(const std::string &name, std::vector<std::pair<std::string, std::string>> &graph);
    std::string GenerateDot(const std::vector<std::pair<std::string, std::string>> &graph);
    std::shared_ptr<InstanceRunHandler> instanceRunHandler;
};
}

#endif

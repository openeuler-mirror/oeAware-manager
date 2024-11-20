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
#ifndef PLUGIN_MGR_EVENT_LIST_HANDLER_H
#define PLUGIN_MGR_EVENT_LIST_HANDLER_H
#include "event_handler.h"
#include "config.h"

namespace oeaware {
class ListHandler : public Handler {
public:
    explicit ListHandler(std::shared_ptr<Config> config) : config(config) { }
    EventResult Handle(const Event &event) override;
private:
    ErrorCode AddList(std::string &res);
    std::string GetPluginInDir(const std::string &path);
    std::shared_ptr<Config> config;
};
}

#endif

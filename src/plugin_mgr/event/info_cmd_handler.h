/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef PLUGIN_MGR_EVENT_INFO_CMD_HANDLER_H
#define PLUGIN_MGR_EVENT_INFO_CMD_HANDLER_H
#include "event_handler.h"
#include "config.h"

namespace oeaware {
struct InfoCmd {
    std::string instance;
    std::string description;
    std::string effect;
};
class InfoCmdHandler : public Handler {
public:
    explicit InfoCmdHandler(std::shared_ptr<Config> config) : config(config) { }
    EventResult Handle(const Event &event) override;
    InfoCmd GetInfo(const std::string& name);
    bool CreateInfoCmdInstances(const std::string& filePath, std::vector<InfoCmd>& infoCmds);
private:
    ErrorCode AddList(std::string &res);
    std::vector<InfoCmd> infoCmd;
    std::shared_ptr<Config> config;
    const size_t instanceWidth = 26;
    const size_t descriptionWidth = 40;
    const size_t effectWidth = 40;
};
}

#endif

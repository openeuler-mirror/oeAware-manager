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
#include "list_handler.h"
#include <dirent.h>
#include <sys/stat.h>
#include "oeaware/utils.h"
#include "oeaware/default_path.h"

namespace oeaware {
std::string ListHandler::GetPluginInDir(const std::string &path)
{
    std::string res;
    struct stat s = {};
    lstat(path.c_str(), &s);
    if (!S_ISDIR(s.st_mode)) {
        return res;
    }
    struct dirent *fileName = nullptr;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return res;
    }
    while ((fileName = readdir(dir)) != nullptr) {
        if (EndWith(std::string(fileName->d_name), ".so")) {
            res += std::string(fileName->d_name) + "\n";
        }
    }
    return res;
}

ErrorCode ListHandler::AddList(std::string &res)
{
    auto pluginList = config->GetPluginList();
    res += "Supported Packages:\n";
    for (auto &p : pluginList) {
        res += p.first + "\n";
    }
    res += "Installed Plugins:\n";
    res += GetPluginInDir(DEFAULT_PLUGIN_PATH);
    return ErrorCode::OK;
}

EventResult ListHandler::Handle(const Event &event)
{
    (void)event;
    std::string resText;
    auto retCode = AddList(resText);
    EventResult eventResult;
    if (retCode == ErrorCode::OK) {
        INFO(logger, "query plugin_list.");
        eventResult.opt = Opt::RESPONSE_OK;
        eventResult.payload.emplace_back(resText);
    } else {
        WARN(logger, "query plugin_list failed, because " << ErrorText::GetErrorText(retCode) << ".");
        eventResult.opt = Opt::RESPONSE_ERROR;
        eventResult.payload.emplace_back(ErrorText::GetErrorText(retCode));
    }
    return eventResult;
}
}

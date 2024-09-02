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
#include "download_handler.h"

namespace oeaware {
ErrorCode DownloadHandler::Download(const std::string &name, std::string &res)
{
    if (!config->IsPluginInfoExist(name)) {
        return ErrorCode::DOWNLOAD_NOT_FOUND;
    }
    res += config->GetPluginInfo(name).GetUrl();
    return ErrorCode::OK;
}

EventResult DownloadHandler::Handle(const Event &event)
{
    std::string resText;
    std::string name = event.GetPayload(0);
    auto retCode = Download(name, resText);
    EventResult eventResult;
    if (retCode == ErrorCode::OK) {
        INFO("[PluginManager] download " << name << " from " << resText << ".");
        eventResult.SetOpt(Opt::RESPONSE_OK);
        eventResult.AddPayload(resText);
    } else {
        WARN("[PluginManager] download " << name << " failed, because " << ErrorText::GetErrorText(retCode) + ".");
        eventResult.SetOpt(Opt::RESPONSE_ERROR);
        eventResult.AddPayload(ErrorText::GetErrorText(retCode));
    }
    return eventResult;
}
}

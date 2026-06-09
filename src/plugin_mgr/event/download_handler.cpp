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
#include <sys/utsname.h>
#include <regex>
namespace oeaware {
// Get kernel major.minor version, e.g., 5.10
std::string GetKernelMajorMinor()
{
    struct utsname buffer;
    if (uname(&buffer) != 0) {
        return "";
    }
    std::string version = buffer.release;
    size_t firstDot = version.find('.');
    if (firstDot == std::string::npos) {
        return "";
    }
    size_t secondDot = version.find('.', firstDot + 1);
    if (secondDot == std::string::npos) {
        return "";
    }
    return version.substr(0, secondDot);
}

EventResult DownloadHandler::Handle(const Event &event)
{
    if (event.payload.empty()) {
        WARN(logger, "download event error.");
        return EventResult(Opt::RESPONSE_ERROR, {"download event error"});
    }
    std::string name = event.payload[0];
    if (!config->GetPluginList().count(name)) {
        WARN(logger, name << " download not found.");
        return EventResult(Opt::RESPONSE_ERROR, {"download not found"});
    }
    auto kernelMajorMinor = GetKernelMajorMinor();
    auto url = config->GetPluginInfo(name).GetUrl();
    if (url.empty() && supportPackageUrl.count(name) &&
        supportPackageUrl[name].count(kernelMajorMinor)) {
        url = supportPackageUrl[name][kernelMajorMinor];
    }
    if (url.empty()) {
        WARN(logger, name << " url is empty.");
        return EventResult(Opt::RESPONSE_ERROR, {"url is empty"});
    }
    EventResult eventResult;
    INFO(logger, "download " << name << " from " << url << ".");
    eventResult.opt = Opt::RESPONSE_OK;
    eventResult.payload.emplace_back(url);
   
    return eventResult;
}
}

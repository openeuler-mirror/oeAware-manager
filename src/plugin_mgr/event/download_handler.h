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
#ifndef PLUGIN_MGR_EVENT_DOWNLOAD_HANDLER_H
#define PLUGIN_MGR_EVENT_DOWNLOAD_HANDLER_H
#include "event_handler.h"
#include "config.h"

namespace oeaware {
class DownloadHandler : public Handler {
public:
    explicit DownloadHandler(std::shared_ptr<Config> config) : config(config) { }
    EventResult Handle(const Event &event) override;
private:
    std::shared_ptr<Config> config;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> supportPackageUrl {
        {
            "numafast",
            {
                {"4.19", "https://repo.oepkgs.net/openEuler/rpm/openEuler-20.03-LTS-SP1/extras/aarch64/Packages/n/numafast-v2.4.1-2.aarch64.rpm"},
                {"5.10", "https://repo.oepkgs.net/openEuler/rpm/openEuler-22.03-LTS-SP4/extras/aarch64/Packages/n/numafast-v2.4.1-2.aarch64.rpm"},
                {"6.6", "https://repo.oepkgs.net/openEuler/rpm/openEuler-24.03-LTS/extras/aarch64/Packages/n/numafast-v2.4.1-2.aarch64.rpm"},
            }
        },
    };
};
}

#endif

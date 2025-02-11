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

#ifndef PRELOAD_TUNE_H
#define PRELOAD_TUNE_H

#include <set>
#include "oeaware/interface.h"
#include "oeaware/default_path.h"

namespace oeaware {
struct ConfigNode {
    std::string appPath;
    std::set<std::string> soPath;
};

class PreloadTune : public Interface {
public:
    PreloadTune();
    ~PreloadTune() override = default;
    Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
private:
    bool LoadConfig();
    bool CheckExistingSo(const ConfigNode& configNode);
    std::string ExtractSoName(const std::string& path);
    std::string ParseLddLine(const std::string& line);
    std::vector<ConfigNode> config;
    std::string configPath = DEFAULT_PRELOAD_PATH;
};
}

#endif // !PRELOAD_TUNE_H

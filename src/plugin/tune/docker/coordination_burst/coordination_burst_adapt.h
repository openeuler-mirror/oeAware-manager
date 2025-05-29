/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef COORDINATION_BURST_ADAPT_H
#define COORDINATION_BURST_ADAPT_H
#include <set>
#include <yaml-cpp/yaml.h>
#include "oeaware/interface.h"

constexpr double DEFAULT_RATIO = 20.0;
constexpr int MAX_RATIO = 20;
constexpr int MIN_RATIO = 1;

class CoordinationBurstAdapt : public oeaware::Interface {
public:
    CoordinationBurstAdapt();
    ~CoordinationBurstAdapt() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
private:
    std::string FixYaml(std::string raw);
    std::string ParseParam(const std::string &param);
    std::string ReadConfig(const std::string &path);
    std::string CheckYamlParam(YAML::Node &parsedConfig);
    std::string ParseYamlRatio(YAML::Node &parsedConfig);
    bool HasSpecialChar(const std::string &str) const;
    bool CheckEnv();
    int GetRatio();
    std::string SetRatio(const int ratioValue);
    int GetSoftQuota(const std::string &cgroupPath);
    bool SetSoftQuota(const int softQuota, const std::string &cgroupPath);
    void GetAllDockers();
    void RollBack(std::set<std::string> &succeedContainers);;
    void UpdateDockerInfo(const std::string &dockerId);
    bool Init();

    int ratio = DEFAULT_RATIO;
    int originRatio = MIN_RATIO - 1;
    bool isDockerListSet = false;
    bool isRatioSet = false;
    std::unordered_map<std::string, std::string> containers;
    std::unordered_map<std::string, std::string> allContainers;
    std::unordered_map<std::string, int> allContainerOriginSoftQuota;
    std::vector<oeaware::Topic> subscribeTopics;
};
#endif // COORDINATION_BURST_ADAPT_H
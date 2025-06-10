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

#ifndef OEAWARE_MANAGER_DOCKER_SMT_H
#define OEAWARE_MANAGER_DOCKER_SMT_H
#include <set>
#include <utility>
#include <oeaware/default_path.h>
#include "oeaware/interface.h"

class BinaryTune : public oeaware::Interface {
public:
    BinaryTune();
    ~BinaryTune() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;
private:
    struct InternalContainer {
        InternalContainer() = default;
        InternalContainer(std::string name, int64_t period, int64_t quota, int64_t burst,
                          std::string cpus, std::vector<int32_t> tasks):id(std::move(name)), cfsPeriodUs(period),
                          cfsQuotaUs(quota), cfsBurstUs(burst), cpus(std::move(cpus)), tasks(std::move(tasks)) {
        }
        std::string id;
        int64_t cfsPeriodUs = 0;
        int64_t cfsQuotaUs = 0;
        int64_t cfsBurstUs = 0;
        std::string cpus;
        std::vector<int32_t> tasks;
    };
    void FindSpecialBin(std::map<int32_t, int64_t> &dockerCpuOnNuma,
                        std::map<int32_t, std::vector<int32_t>> &threadWaitTuneByNuma,
                        std::map<int32_t, std::vector<std::pair<int32_t, std::vector<int32_t>>>> &threadWaitTuneByCore);
    void SetAffinity(int32_t tid, cpu_set_t& newMask);
    void BindCpuCore(int32_t numaNode,
                     std::map<int32_t, std::vector<std::pair<int32_t, std::vector<int32_t>>>> &threadWaitTuneByCore);
    void BindNuma(int32_t numaNode,
                  std::map<int32_t, std::vector<int32_t>> &threadWaitTuneByNuma);
    void BindAllCores(const std::vector<int32_t> &threads);
    bool ReadConfig(const std::string &path);
    void ParseBinaryElf(const std::string &filePath, int32_t &policy);
    std::vector<oeaware::Topic> subscribeTopics{};
    bool envInit = false;
    std::vector<int> cpu2Numa{};
    int32_t numaNum = 1;
    int32_t cpuCoreNum = 1;
    int32_t cpuNumOnline = 1;
    bool isCpuAllOnline = true;
    std::vector<InternalContainer> containers{};
    std::map<int32_t, std::string> threads{};
    std::map<int32_t, cpu_set_t> tuneThreads{};
    std::map<std::string, int32_t> configBinary{};
    std::vector<cpu_set_t> phyNumaMask{};
    const std::string configPath = oeaware::DEFAULT_PLUGIN_CONFIG_PATH + "/binary_tune.yaml";
    std::set<int32_t> hasLoggedThreads;
};

#endif //OEAWARE_MANAGER_DOCKER_SMT_H

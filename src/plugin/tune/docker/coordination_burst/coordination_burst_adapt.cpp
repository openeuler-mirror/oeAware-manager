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
#include "coordination_burst_adapt.h"
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <regex>
#include <oeaware/default_path.h>
#include "oeaware/utils.h"

constexpr int PERIOD = 1000;
constexpr int PRIORITY = 2;
const std::string SCHED_SOFT_RUNTIME_RATIO_PATH =
    "/proc/sys/kernel/sched_soft_runtime_ratio";
const std::string DOCKER_COORDINATION_BURST_CONFIG_PATH =
    oeaware::DEFAULT_PLUGIN_CONFIG_PATH + "/docker_burst.yaml";

CoordinationBurstAdapt::CoordinationBurstAdapt()
{
    name = OE_DOCKER_COORDINATION_BURST_TUNE;
    description = "";
    version = "1.0.0";
    period = PERIOD;
    priority = PRIORITY;
    type = oeaware::TUNE;
    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = this->name;
    supportTopics.push_back(topic);
}

oeaware::Result CoordinationBurstAdapt::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void CoordinationBurstAdapt::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void CoordinationBurstAdapt::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

void CoordinationBurstAdapt::UpdateDockerInfo(const std::string &dockerId)
{
    if (dockerId.length() == oeaware::DockerUtils::LONG_DOCKER_ID_LENGTH) {
        if (allContainers.find(dockerId) != allContainers.end()) {
            containers[dockerId] = allContainers[dockerId];
        } else {
            ERROR(logger, "dockerId " << dockerId << " is not found."
                          " Cannot modify docker coordination burst config.");
        }
        return;
    }
    if (dockerId.length() == oeaware::DockerUtils::SHORT_DOCKER_ID_LENGTH) {
        for (auto &dockerIt : allContainers) {
            if (dockerIt.first.substr(0, oeaware::DockerUtils::SHORT_DOCKER_ID_LENGTH) == dockerId) {
                containers[dockerIt.first] = dockerIt.second;
                return;
            }
        }
        ERROR(logger, "dockerId " << dockerId << " is not found."
                      " Cannot modify docker coordination burst config.");
    }
}

std::string CoordinationBurstAdapt::FixYaml(std::string raw)
{
    raw = std::regex_replace(raw, std::regex(":"), ": ");
    raw = std::regex_replace(raw, std::regex(",\\s*"), ", ");
    return raw;
}

int CoordinationBurstAdapt::ParseParam(const std::string &param)
{
    std::string paramStr = FixYaml("{" + param + "}");
    std::istringstream iss(paramStr);
    try {
        YAML::Node parsedConfig = YAML::Load(iss);
        if (parsedConfig["ratio"]) {
            ratio = parsedConfig["ratio"].as<double>();
            isRatioSet = true;
        }
        if (parsedConfig["docker_id"]) {
            if (parsedConfig["docker_id"].IsSequence()) {
                for (auto &&id : parsedConfig["docker_id"]) {
                    UpdateDockerInfo(id.as<std::string>());
                }
            } else {
                UpdateDockerInfo(parsedConfig["docker_id"].as<std::string>());
            }
            isDockerListSet = true;
        }
    } catch (const YAML::Exception &e) {
        ERROR(logger, "CoordinationBurstAdapt ParseParam config format is invalid.");
        return -1;
    }
    return 0;
}

int CoordinationBurstAdapt::ReadConfig(const std::string &path)
{
    try {
        std::ifstream sysFile(path);

        if (!sysFile.is_open()) {
            WARN(logger, "docker_burst.yaml config open failed.");
            return -1;
        }

        YAML::Node config = YAML::LoadFile(path);

        if (!isRatioSet) {
            double paramRatio = config["ratio"].as<double>();
            ratio = paramRatio;
            isRatioSet = true;
        }

        std::vector<std::string> dockerList;
        if (!isDockerListSet && config["docker_list"] && config["docker_list"].IsSequence()) {
            dockerList = config["docker_list"].as<std::vector<std::string>>();
            for (const auto &dockerId : dockerList) {
                UpdateDockerInfo(dockerId);
            }
            isDockerListSet = dockerList.size() == 0 ? false : true;
        }

        sysFile.close();
    } catch (const YAML::Exception &e) {
        std::cerr << "YAML parse failed: " << e.what() << std::endl;
    }
    return 0;
}

void CoordinationBurstAdapt::GetAllDockers()
{
    allContainers.clear();
    allContainerOriginSoftQuota.clear();
    std::vector<std::string> dockerList = oeaware::DockerUtils::GetAllDockerIDs();
    for (const auto &dockerId : dockerList) {
        std::string dockerCgroupPath =
            oeaware::DockerUtils::FindDockerSubsystemCgroupPath(dockerId, "cpu,cpuacct");
        if (!dockerCgroupPath.empty()) {
            allContainers[dockerId] = dockerCgroupPath;
            allContainerOriginSoftQuota[dockerId] = GetSoftQuota(dockerCgroupPath);
        } else {
            WARN(logger, "dockerId " << dockerId << " can not find cgroup path.");
        }
    }
}

void CoordinationBurstAdapt::RollBack(std::set<std::string> &succeedContainers)
{
    SetRatio(originRatio);
    for (auto &container : succeedContainers) {
        auto originSoftQuota = allContainerOriginSoftQuota[container];
        SetSoftQuota(originSoftQuota, allContainers[container]);
    }
}

oeaware::Result CoordinationBurstAdapt::Enable(const std::string &param)
{
    if (!Init()) {
        return oeaware::Result(FAILED, "Docker coordination burst init failed. may not support.");
    }

    ParseParam(param);
    if (!isDockerListSet || !isRatioSet) {
        ReadConfig(DOCKER_COORDINATION_BURST_CONFIG_PATH);
    }

    if (containers.empty()) {
        if (!isDockerListSet) {
            containers.insert(allContainers.begin(), allContainers.end());
        } else {
            return oeaware::Result(FAILED, "All config docker_id is invalid."
                                           "Cannot found cgroup path or docker not exist.");
        }
    }

    if (!SetRatio(ratio)) {
        return oeaware::Result(FAILED, "Set ratio to " + DOCKER_COORDINATION_BURST_CONFIG_PATH + " failed.");
    }

    std::set<std::string> succeedContainers;
    for (auto &container : containers) {
        // Set cpu.soft_quota = 1 to enable docker coordination burst
        if (SetSoftQuota(1, container.second)) {
            succeedContainers.insert(container.first);
        } else {
            RollBack(succeedContainers);
            return oeaware::Result(FAILED, "Set soft quota to " + container.first + " failed.");
        }
    }

    return oeaware::Result(OK);
}

void CoordinationBurstAdapt::Disable()
{
    if (!CheckEnv()) {
        ERROR(logger, "Docker CoordinationBurst check env failed.");
        return;
    }

    SetRatio(originRatio);
    for (auto &container : allContainers) {
        auto originSoftQuota = allContainerOriginSoftQuota[container.first];
        SetSoftQuota(originSoftQuota, allContainers[container.first]);
    }
}

void CoordinationBurstAdapt::Run()
{
}

bool CoordinationBurstAdapt::CheckEnv()
{
    std::ifstream inFile(SCHED_SOFT_RUNTIME_RATIO_PATH);
    return inFile.good();
}

int CoordinationBurstAdapt::GetRatio()
{
    int ratioValue = MIN_RATIO - 1;
    std::ifstream file(SCHED_SOFT_RUNTIME_RATIO_PATH, std::ios::out | std::ios::in);
    if (!file.is_open()) {
        ERROR(logger, "Failed to open file: " << SCHED_SOFT_RUNTIME_RATIO_PATH);
        return ratioValue;
    }

    file >> ratioValue;
    file.close();

    if (file.fail()) {
        ERROR(logger, "Failed to read value from file: " << SCHED_SOFT_RUNTIME_RATIO_PATH);
    }

    return ratioValue;
}

bool CoordinationBurstAdapt::SetRatio(const int ratioValue)
{
    if (ratioValue < MIN_RATIO || ratioValue > MAX_RATIO) {
        ERROR(logger, "ratio is invalid. The ratio must be between " << MIN_RATIO <<
                      " and " << std::to_string(MAX_RATIO));
        return  false;
    }

    std::ofstream file(SCHED_SOFT_RUNTIME_RATIO_PATH, std::ios::out | std::ios::in);
    if (!file.is_open()) {
        ERROR(logger, "Failed to open file: " << SCHED_SOFT_RUNTIME_RATIO_PATH);
        return false;
    }
    file << ratioValue;
    if (!file.flush()) {
        ERROR(logger, "Failed to flush << " << ratioValue << " to file: " << SCHED_SOFT_RUNTIME_RATIO_PATH);
        return false;
    }
    file.close();
    INFO(logger, "Successfully set " << SCHED_SOFT_RUNTIME_RATIO_PATH << " to: " << ratioValue);
    return true;
}

int CoordinationBurstAdapt::GetSoftQuota(const std::string &cgroupPath)
{
    int softQuotaValue = -1;
    std::string fileName = cgroupPath + "/cpu.soft_quota";
    std::ifstream file(fileName);
    if (!file.is_open()) {
        ERROR(logger, "failed to open " << fileName);
        return softQuotaValue;
    }

    file >> softQuotaValue;
    file.close();

    if (file.fail()) {
        ERROR(logger, "Failed to read value from file: " << fileName);
        return softQuotaValue;
    }

    return softQuotaValue;
}

bool CoordinationBurstAdapt::SetSoftQuota(const int softQuota, const std::string &cgroupPath)
{
    std::string fileName = cgroupPath + "/cpu.soft_quota";
    std::ofstream file(fileName);
    if (!file.is_open()) {
        ERROR(logger, "failed to open " << fileName);
        return false;
    }

    file << softQuota;
    if (!file.flush()) {
        ERROR(logger, "Failed to flush << " << softQuota << " to file: " << fileName);
        return false;
    }
    file.close();
    return true;
}

bool CoordinationBurstAdapt::Init()
{
    if (!CheckEnv()) {
        return false;
    }

    GetAllDockers();
    originRatio = GetRatio();
    isDockerListSet = false;
    isRatioSet = false;
    containers.clear();
    ratio = DEFAULT_RATIO;

    return true;
}
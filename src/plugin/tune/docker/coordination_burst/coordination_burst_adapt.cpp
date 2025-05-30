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

std::string CoordinationBurstAdapt::CheckYamlParam(YAML::Node &parsedConfig)
{
    std::string retMsg;
    if (!parsedConfig.IsMap()) {
        retMsg = "CoordinationBurstAdapt ParseParam config format is invalid.";
        ERROR(logger, retMsg);
        return retMsg;
    }

    std::string invalidKeys = "";
    for (const auto &pair : parsedConfig) {
        std::string key = pair.first.as<std::string>();
        if (key != "ratio" && key != "docker_id") {
            invalidKeys += key + " ";
        }
    }

    if (invalidKeys.empty()) {
        return "";
    }
    retMsg = "CoordinationBurstAdapt ParseParam " + invalidKeys + "is invalid."
             " Please use cli param '-docker_id [id1,id2] -ratio 20' or set"
             " docker_id and ratio in " + DOCKER_COORDINATION_BURST_CONFIG_PATH;
    ERROR(logger, retMsg);
    return retMsg;
}

std::string CoordinationBurstAdapt::ParseYamlRatio(YAML::Node &parsedConfig)
{
    std::string retMsg;
    std::string ratioString = parsedConfig["ratio"].as<std::string>();
    if (!oeaware::IsInteger(ratioString)) {
        retMsg = "CoordinationBurstAdapt Param ratio should be integer.";
        ERROR(logger, retMsg);
        return retMsg;
    } else {
        ratio = parsedConfig["ratio"].as<int>();
        isRatioSet = true;
    }
    return "";
}

bool CoordinationBurstAdapt::HasSpecialChar(const std::string &str) const
{
    return str.find_first_not_of(
        "abcdefjhijklmnopqrstuvwxyz"
        "0123456789:_[],. {}") != std::string::npos;
}

std::string CoordinationBurstAdapt::ParseParam(const std::string &param)
{
    std::string retMsg;
    std::string paramStr = FixYaml("{" + param + "}");
    if (HasSpecialChar(paramStr)) {
        retMsg = "Input param has invalid character.";
        return retMsg;
    }
    std::istringstream iss(paramStr);
    try {
        YAML::Node parsedConfig = YAML::Load(iss);
        retMsg = CheckYamlParam(parsedConfig);
        if (retMsg != "") {
            return retMsg;
        }

        if (parsedConfig["ratio"]) {
            retMsg = ParseYamlRatio(parsedConfig);
            if (retMsg != "") {
                return retMsg;
            }
        }
        if (parsedConfig["docker_id"]) {
            if (parsedConfig["docker_id"].IsSequence()) { // param: -docker_id [id1,id2,id3]
                for (auto &&id : parsedConfig["docker_id"]) {
                    UpdateDockerInfo(id.as<std::string>());
                }
            } else { // param: -docker_id id1
                UpdateDockerInfo(parsedConfig["docker_id"].as<std::string>());
            }
            isDockerListSet = true;
        }
    } catch (const YAML::Exception &e) {
        retMsg = "CoordinationBurstAdapt ParseParam format parse failed: " + std::string(e.what());
        ERROR(logger, retMsg);
        return retMsg;
    }
    return "";
}

std::string CoordinationBurstAdapt::ReadConfig(const std::string &path)
{
    std::string retMsg;
    try {
        std::ifstream sysFile(path);

        if (!sysFile.is_open()) {
            retMsg = DOCKER_COORDINATION_BURST_CONFIG_PATH + " config open failed.";
            WARN(logger, retMsg);
            return retMsg;
        }

        YAML::Node config = YAML::LoadFile(path);

        retMsg = CheckYamlParam(config);
        if (retMsg != "") {
            return retMsg;
        }

        if (!isRatioSet && config["ratio"]) {
            retMsg = ParseYamlRatio(config);
            if (retMsg != "") {
                return retMsg;
            }
        }

        std::vector<std::string> dockerList;
        if (!isDockerListSet && config["docker_id"] && config["docker_id"].IsSequence()) {
            dockerList = config["docker_id"].as<std::vector<std::string>>();
            for (const auto &dockerId : dockerList) {
                UpdateDockerInfo(dockerId);
            }
            isDockerListSet = dockerList.size() == 0 ? false : true;
        }

        sysFile.close();
    } catch (const YAML::Exception &e) {
        retMsg = "CoordinationBurstAdapt read config from " + DOCKER_COORDINATION_BURST_CONFIG_PATH
                 + " YAML parse failed: " + std::string(e.what());
        ERROR(logger, retMsg);
        return retMsg;
    }
    return "";
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

    std::string retMsg = ParseParam(param);
    if (!retMsg.empty()) {
        return oeaware::Result(FAILED, retMsg);
    }
    if (!isDockerListSet || !isRatioSet) {
        retMsg = ReadConfig(DOCKER_COORDINATION_BURST_CONFIG_PATH);
        if (!retMsg.empty()) {
            return oeaware::Result(FAILED, retMsg);
        }
    }

    if (containers.empty()) {
        if (!isDockerListSet) {
            containers.insert(allContainers.begin(), allContainers.end());
        } else {
            return oeaware::Result(FAILED,
                "All config docker_id is invalid."
                "Cannot found cgroup path or docker not exist.");
        }
    }

    retMsg = SetRatio(ratio);
    if (retMsg != "") {
        return oeaware::Result(FAILED, retMsg);
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

std::string CoordinationBurstAdapt::SetRatio(const int ratioValue)
{
    std::string retMsg;
    if (ratioValue < MIN_RATIO || ratioValue > MAX_RATIO) {
        retMsg = "ratio is invalid. The ratio must be in [" + std::to_string(MIN_RATIO) +
                 ", " + std::to_string(MAX_RATIO) + "].";
        ERROR(logger, retMsg);
        return retMsg;
    }

    std::ofstream file(SCHED_SOFT_RUNTIME_RATIO_PATH, std::ios::out | std::ios::in);
    if (!file.is_open()) {
        retMsg = "Failed to open file: " + SCHED_SOFT_RUNTIME_RATIO_PATH + ".";
        ERROR(logger, retMsg);
        return retMsg;
    }
    file << ratioValue;
    if (!file.flush()) {
        retMsg = "Failed to flush " + std::to_string(ratioValue) + " to file: " +
                 SCHED_SOFT_RUNTIME_RATIO_PATH + ".";
        ERROR(logger, retMsg);
        return retMsg;
    }
    file.close();
    INFO(logger, "Successfully set " << SCHED_SOFT_RUNTIME_RATIO_PATH << " to: " << ratioValue);
    return "";
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
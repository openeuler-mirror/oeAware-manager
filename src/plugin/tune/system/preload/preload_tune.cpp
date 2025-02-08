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

#include <unistd.h>
#include <sys/stat.h>
#include <yaml-cpp/yaml.h>
#include "preload_tune.h"

using namespace oeaware;

PreloadTune::PreloadTune()
{
    name = OE_PRELOAD_TUNE;
    version = "1.0.0";
    period = 1000; // 1000 period is meaningless, only Enable() is executed, not Run()
    priority = 2;
    type = TUNE;
    description = "configure shared object required by application and automatically load it";
}

oeaware::Result PreloadTune::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void PreloadTune::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void PreloadTune::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

bool PreloadTune::LoadConfig()
{
    struct stat buffer;
    if (stat(configPath.c_str(), &buffer) != 0) {
        return false;
    }

    try {
        YAML::Node node = YAML::LoadFile(configPath);
        if (node.IsNull()) {
            WARN(logger, "empty preload YAML file");
            return true;
        }
        if (!node.IsSequence()) {
            ERROR(logger, "wrong format in preload config file.");
            return false;
        }

        for (const auto& app : node) {
            if (!app["apppath"].IsDefined() || !app["sopath"].IsDefined()) {
                ERROR(logger, "wrong format in preload config file.");
                return false;
            }

            ConfigNode configNode;
            configNode.appPath = app["apppath"].as<std::string>();
            if (access(configNode.appPath.c_str(), F_OK) != 0) {
                ERROR(logger, "app not found: " << configNode.appPath);
                return false;
            }
            if (!app["sopath"].IsSequence()) {
                ERROR(logger, "wrong sopath format in preload config file.");
                return false;
            }

            for (const auto& so : app["sopath"]) {
                std::string soPath = so.as<std::string>();
                if (access(soPath.c_str(), F_OK) != 0) {
                    ERROR(logger, "shared object not found: " << soPath);
                    return false;
                }

                auto result = configNode.soPath.insert(std::move(soPath));
                if (!result.second) {
                    ERROR(logger, "duplicate shared object: " << soPath);
                    return false;
                }
            }
            config.push_back(std::move(configNode));
        }
        INFO(logger, "parse preload YAML file success");
        return true;
    } catch (const YAML::Exception& e) {
        ERROR(logger, "parse preload YAML file failed");
        return false;
    }
}

std::string PreloadTune::ExtractSoName(const std::string& path)
{
    std::string soName;
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos) {
        soName = path.substr(lastSlash + 1);
    } else {
        soName = path;
    }

    soName.erase(0, soName.find_first_not_of(" \t"));
    soName.erase(soName.find_last_not_of(" \t") + 1);

    return soName;
}

// 处理可能的几种格式：
// 1. libxxx.so => /path/to/libxxx.so (0x0000xxxxxxxxxxxx)
// 2. libxxx.so (0x0000xxxxxxxxxxxx)
// 3. /path/to/libxxx.so (0x0000xxxxxxxxxxxx)
std::string PreloadTune::ParseLddLine(const std::string& line)
{
    if (line.find(".so") == std::string::npos) {
        return "";
    }

    std::string soName = line;

    size_t arrowPos = line.find("=>");
    if (arrowPos != std::string::npos) {
        soName = line.substr(0, arrowPos);
    } else {
        size_t bracketPos = line.find(" (");
        if (bracketPos != std::string::npos) {
            soName = line.substr(0, bracketPos);
        }
    }

    return ExtractSoName(soName);
}

bool PreloadTune::CheckExistingSo(const ConfigNode& configNode)
{
    std::set<std::string> existingSoList;
    std::string cmd = "ldd " + configNode.appPath;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        ERROR(logger, "failed to execute ldd command for " << configNode.appPath);
        return false;
    }

    char buffer[256];
    bool hasValidSo = false;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string soName = ParseLddLine(buffer);
        if (soName.empty()) {
            continue;
        }
        hasValidSo = true;
        existingSoList.insert(soName);
    }
    pclose(pipe);

    if (!hasValidSo) {
        ERROR(logger, "failed to parse ldd output for " << configNode.appPath);
        return false;
    }

    for (const auto& soPath : configNode.soPath) {
        std::string soName = ExtractSoName(soPath);
        if (existingSoList.find(soName) != existingSoList.end()) {
            ERROR(logger, "shared object " << soName << " already exists in " << configNode.appPath);
            return false;
        }
    }

    return true;
}

oeaware::Result PreloadTune::Enable(const std::string &param)
{
    (void)param;
    if (!oeaware::FileExist(configPath)) {
        return oeaware::Result(FAILED, "preload config file does not exist");
    }
    if (!oeaware::CheckPermission(configPath, S_IRUSR | S_IWUSR | S_IRGRP)) {
        return oeaware::Result(FAILED, "Insufficient permission on " + configPath);
    }
    if (!LoadConfig()) {
        config.clear();
        return oeaware::Result(FAILED, "load preload config file failed");
    }

    for (const auto& configNode : config) {
        if (!CheckExistingSo(configNode)) {
            std::string appPath = configNode.appPath;
            config.clear();
            return oeaware::Result(FAILED, "conflicting shared object in " + appPath);
        }
    }

    for (const auto& configNode : config) {
        for (const auto& so : configNode.soPath) {
            std::string cmd = "patchelf --add-needed " + so + " " + configNode.appPath;
            int ret = system(cmd.c_str());
            if (ret != 0) {
                config.clear();
                return oeaware::Result(FAILED, "failed to execute patchelf command: " + cmd);
            }
        }
    }

    return oeaware::Result(OK);
}

void PreloadTune::Disable()
{
    for (const auto& app : config) {
        for (const auto& so : app.soPath) {
            std::string cmd = "patchelf --remove-needed " + so + " " + app.appPath;
            int ret = system(cmd.c_str());
            if (ret != 0) {
                WARN(logger, "failed to execute patchelf command: " << cmd);
            }
        }
    }

    config.clear();
}

void PreloadTune::Run()
{
}

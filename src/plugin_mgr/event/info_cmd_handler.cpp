/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include <dirent.h>
#include <sys/stat.h>
#include <yaml-cpp/yaml.h>
#include <sstream>
#include <iomanip>
#include "oeaware/utils.h"
#include "oeaware/default_path.h"
#include "info_cmd_handler.h"

namespace oeaware {
bool InfoCmdHandler::CreateInfoCmdInstances(const std::string& filePath, std::vector<InfoCmd>& infoCmds)
{
    struct stat buffer;
    if (stat(filePath.c_str(), &buffer) != 0) {
        ERROR(logger, "File does not exist or cannot be accessed: " << filePath);
        return false;
    }

    try {
        YAML::Node node = YAML::LoadFile(filePath);
        if (node.IsNull()) {
            WARN(logger, "Empty YAML file");
            return true;
        }
        if (!node["InfoCmd"].IsDefined() || !node["InfoCmd"].IsSequence()) {
            ERROR(logger, "Wrong format in config file.");
            return false;
        }

        for (const auto& cmd : node["InfoCmd"]) {
            if (!cmd["instance"].IsDefined() || !cmd["description"].IsDefined() || !cmd["effect"].IsDefined()) {
                ERROR(logger, "Missing required fields in 'InfoCmd' node.");
                return false;
            }

            InfoCmd infoCmd {
                cmd["instance"].as<std::string>(),
                cmd["description"].as<std::string>(),
                cmd["effect"].as<std::string>()
            };

            infoCmds.push_back(infoCmd);
        }

        INFO(logger, "Parse YAML file success");
        return true;
    } catch (const YAML::Exception& e) {
        ERROR(logger, "Failed to parse YAML file: " << e.what());
        return false;
    }
}

InfoCmd InfoCmdHandler::GetInfo(const std::string& name)
{
    for (const auto& curInfo : infoCmd) {
        if (curInfo.instance == name) {
            return curInfo;
        }
    }
    InfoCmd curInfo{name, "", ""};
    return curInfo;
}

std::vector<std::string> wrapText(const std::string& text, size_t width)
{
    std::vector<std::string> wrapped;
    size_t start = 0;
    while (start != std::string::npos && start < text.length()) {
        if (text.length() - start <= width) {
            wrapped.push_back(text.substr(start));
            break;
        }
        size_t end = text.find_last_of(" ", start + width);
        if (end == std::string::npos || end < start) {
            end = text.find_first_of(" ", start);
            if (end == std::string::npos) {
                wrapped.push_back(text.substr(start));
                break;
            }
        }
        wrapped.push_back(text.substr(start, end - start));
        start = end + 1;
        while (start < text.length() && text[start] == ' ') {
            ++start;
        }
    }
    return wrapped;
}

void InfoCmdHandler::FormatAndPrint(const std::string& instanceName, const std::string& description,
                                    const std::string& effect, std::ostringstream &formattedRes)
{
    auto instanceNames = wrapText(instanceName, instanceWidth);
    auto descriptions = wrapText(description, descriptionWidth);
    auto effects = wrapText(effect, effectWidth);
    size_t maxLines = std::max({descriptions.size(), effects.size(), static_cast<size_t>(1)});
    for (size_t j = 0; j < maxLines; ++j) {
            if (j < instanceNames.size()) {
            formattedRes << std::left << std::setw(instanceWidth) << instanceNames[j];
        } else {
            formattedRes << std::left << std::setw(instanceWidth) << "";
        }
        if (j < descriptions.size()) {
            formattedRes << "|" << std::setw(descriptionWidth) << descriptions[j];
        } else {
            formattedRes << "|" << std::setw(descriptionWidth) << "";
        }
        if (j < effects.size()) {
            formattedRes << "|" << std::setw(effectWidth) << effects[j];
        } else {
            formattedRes << "|" << std::setw(effectWidth) << "";
        }
        formattedRes << "\n";
    }
    formattedRes << std::string(instanceWidth + 1 + descriptionWidth + 1 + effectWidth, '-') << "\n";
}

ErrorCode InfoCmdHandler::AddList(std::string &res)
{
    std::vector<std::shared_ptr<Plugin>> allPlugins = memoryStore->GetAllPlugins();
    std::ostringstream formattedRes;
    if (!CreateInfoCmdInstances("/etc/oeAware/info_cmd.yaml", infoCmd)) {
        return ErrorCode::LOAD_PLUGIN_FILE_NOT_EXIST;
    }
    formattedRes << std::left << std::setw(instanceWidth) << "Instance" << "|" <<
                 std::setw(descriptionWidth) << "Description" << "|" <<
                 std::setw(effectWidth) << "Effect" << "\n";
    formattedRes << std::string(instanceWidth + 1 + descriptionWidth + 1 + effectWidth, '-') << "\n";
    std::vector<std::shared_ptr<Plugin>> tunePlugins;
    for (auto &p : allPlugins) {
        if (p->GetName().find("_tune")!=std::string::npos) {
            tunePlugins.push_back(p);
        }
    }
    for (auto &p : tunePlugins) {
        for (size_t i = 0; i < p->GetInstanceLen(); ++i) {
                auto instance = p->GetInstance(i);
                auto infoPartner = GetInfo(instance->GetName());
                std::string instanceName = instance->GetRun();
                std::string description = infoPartner.description;
                std::string effect = infoPartner.effect;
                FormatAndPrint(instanceName, description, effect, formattedRes);
            }
    }
    res = formattedRes.str();
    return ErrorCode::OK;
}

EventResult InfoCmdHandler::Handle(const Event &event)
{
    (void)event;
    std::string resText;
    auto retCode = AddList(resText);
    EventResult eventResult;
    if (retCode == ErrorCode::OK) {
        INFO(logger, "InfoCmd query plugin_list.");
        eventResult.opt = Opt::RESPONSE_OK;
        eventResult.payload.emplace_back(resText);
    } else {
        WARN(logger, "InfoCmd query plugin_list failed, because " << ErrorText::GetErrorText(retCode) << ".");
        eventResult.opt = Opt::RESPONSE_ERROR;
        eventResult.payload.emplace_back(ErrorText::GetErrorText(retCode));
    }
    return eventResult;
}
}

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
#include "load_handler.h"
#include <sys/stat.h>
#include "utils.h"
#include "default_path.h"

namespace oeaware {
void LoadHandler::ConstructLackDep(const std::vector<std::vector<std::string>> &query, std::string &res)
{
    std::vector<std::string> lack;
    size_t nodeCnt = 2;
    for (auto &item : query) {
        if (item.size() < nodeCnt) continue;
        if (!memoryStore->IsInstanceExist(item[1])) {
            lack.emplace_back(item[1]);
        }
    }
    if (!lack.empty()) {
        for (size_t j = 0; j < lack.size(); ++j) {
            res += "\t" + lack[j];
            if (j != lack.size() - 1) res += '\n';
        }
    }
}

std::string LoadHandler::InstanceDepCheck(const std::string &name)
{
    std::shared_ptr<Plugin> plugin = memoryStore->GetPlugin(name);
    std::string res;
    for (size_t i = 0; i < plugin->GetInstanceLen(); ++i) {
        std::string instanceName = plugin->GetInstance(i)->GetName();
        std::vector<std::vector<std::string>> query;
        memoryStore->QueryNodeDependency(instanceName, query);
        ConstructLackDep(query, res);
    }
    return res;
}

int LoadHandler::LoadDlInstance(std::shared_ptr<Plugin> plugin, Interface **interfaceList)
{
    int (*getInstance)(Interface**) = (int(*)(Interface**))dlsym(plugin->GetHandler(), "get_instance");
    if (getInstance == nullptr) {
        ERROR(logger, "dlsym error!");
        return -1;
    }
    int len = getInstance(interfaceList);
    DEBUG(logger, "dl loaded!");
    return len;
}

void LoadHandler::SaveInstance(std::shared_ptr<Plugin> plugin, Interface *interfaceList, int len)
{
    if (interfaceList == nullptr) return;
    for (int i = 0; i < len; ++i) {
        Interface *interface = interfaceList + i;
        std::shared_ptr<Instance> instance = std::make_shared<Instance>();
        std::string name = interface->get_name();
        instance->SetInterface(interface);
        instance->SetName(name);
        instance->SetPluginName(plugin->GetName());
        instance->SetEnabled(false);
        DEBUG(logger, "Instance: " << name.c_str());
        memoryStore->AddInstance(instance);
        plugin->AddInstance(instance);
    }
}

bool LoadHandler::LoadInstance(std::shared_ptr<Plugin> plugin)
{
    int len = 0;
    DEBUG(logger, "plugin: " << plugin->GetName());
    Interface *interfaceList = nullptr;
    len = LoadDlInstance(plugin, &interfaceList);
    if (len < 0) {
        return false;
    }
    SaveInstance(plugin, interfaceList, len);
    return true;
}

ErrorCode LoadHandler::LoadPlugin(const std::string &name)
{
    std::string plugin_path = DEFAULT_PLUGIN_PATH + "/" + name;
    if (!FileExist(plugin_path)) {
        return ErrorCode::LOAD_PLUGIN_FILE_NOT_EXIST;
    }
    if (!EndWith(name, ".so")) {
        return ErrorCode::LOAD_PLUGIN_FILE_IS_NOT_SO;
    }
    if (!CheckPermission(plugin_path, S_IRUSR | S_IRGRP)) {
        return ErrorCode::LOAD_PLUGIN_FILE_PERMISSION_DEFINED;
    }
    if (memoryStore->IsPluginExist(name)) {
        return ErrorCode::LOAD_PLUGIN_EXIST;
    }
    std::shared_ptr<Plugin> plugin = std::make_shared<Plugin>(name);
    int error = plugin->Load(plugin_path);
    if (error) {
        return ErrorCode::LOAD_PLUGIN_DLOPEN_FAILED;
    }
    if (!this->LoadInstance(plugin)) {
        return ErrorCode::LOAD_PLUGIN_DLSYM_FAILED;
    }
    memoryStore->AddPlugin(name, plugin);
    return ErrorCode::OK;
}

EventResult LoadHandler::Handle(const Event &event)
{
    std::string name = event.GetPayload(0);
    auto retCode = LoadPlugin(name);
    EventResult eventResult;
    if (retCode == ErrorCode::OK) {
        INFO(logger, name << " plugin loaded.");
        auto lackDep = InstanceDepCheck(name);
        eventResult.SetOpt(Opt::RESPONSE_OK);
        if (!lackDep.empty() && event.GetType() == EventType::EXTERNAL) {
            INFO(logger, name << " requires the following dependencies:\n" << lackDep);
            eventResult.AddPayload(lackDep);
        }
    } else {
        WARN(logger, name << " " << ErrorText::GetErrorText(retCode)  << ".");
        eventResult.SetOpt(Opt::RESPONSE_ERROR);
        eventResult.AddPayload(ErrorText::GetErrorText(retCode));
    }
    return eventResult;
}
}

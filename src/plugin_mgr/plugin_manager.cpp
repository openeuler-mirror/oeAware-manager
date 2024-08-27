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
#include "plugin_manager.h"
#include <csignal>
#include <dirent.h>
#include "utils.h"
#include "default_path.h"

namespace oeaware {
void PrintHelp()
{
    printf("Usage: ./oeaware [path]\n"
            "      ./oeaware --help\n"
           "Examples:\n"
           "    ./oeaware /etc/oeAware/config.yaml\n");
}

void SignalHandler(int signum)
{
    (void)signum;
    auto &pluginManager = oeaware::PluginManager::GetInstance();
    pluginManager.SendMsg(oeaware::Message(oeaware::Opt::SHUTDOWN, oeaware::MessageType::INTERNAL));
}

void PluginManager::Init(std::shared_ptr<Config> config, std::shared_ptr<SafeQueue<Message>> handlerMsg,
    std::shared_ptr<SafeQueue<Message>> resMsg)
{
    this->config = config;
    this->handlerMsg = handlerMsg;
    this->resMsg = resMsg;
    instanceRunHandler.reset(new InstanceRunHandler(memoryStore));
}

ErrorCode PluginManager::Remove(const std::string &name)
{
    if (!memoryStore.IsPluginExist(name)) {
        return ErrorCode::REMOVE_PLUGIN_NOT_EXIST;
    }
    std::shared_ptr<Plugin> plugin = memoryStore.GetPlugin(name);
    std::vector<std::string> instanceNames;
    for (size_t i = 0; i < plugin->GetInstanceLen(); ++i) {
        std::shared_ptr<Instance> instance = plugin->GetInstance(i);
        std::string iname = instance->GetName();
        if (instance->GetEnabled()) {
            return ErrorCode::REMOVE_INSTANCE_IS_RUNNING;
        }
        if (memoryStore.HaveDep(iname)) {
            return ErrorCode::REMOVE_INSTANCE_HAVE_DEP;
        }
        instanceNames.emplace_back(iname);
    }
    for (auto &iname : instanceNames) {
        memoryStore.DeleteInstance(iname);
    }
    memoryStore.DeletePlugin(name);
    return ErrorCode::OK;
}

ErrorCode PluginManager::QueryAllPlugins(std::string &res)
{
    std::vector<std::shared_ptr<Plugin>> allPlugins = memoryStore.GetAllPlugins();
    for (auto &p : allPlugins) {
        res += p->GetName() + "\n";
        for (size_t i = 0; i < p->GetInstanceLen(); ++i) {
            std::string info = p->GetInstance(i)->GetInfo();
            res += "\t" + info + "\n";
        }
    }
    return ErrorCode::OK;
}

ErrorCode PluginManager::QueryPlugin(const std::string &name, std::string &res)
{
    if (!memoryStore.IsPluginExist(name)) {
        return ErrorCode::QUERY_PLUGIN_NOT_EXIST;
    }
    std::shared_ptr<Plugin> plugin = memoryStore.GetPlugin(name);
    res += name + "\n";
    for (size_t i = 0; i < plugin->GetInstanceLen(); ++i) {
        std::string info = plugin->GetInstance(i)->GetInfo();
        res += "\t" + info + "\n";
    }
    return ErrorCode::OK;
}

int PluginManager::LoadDlInstance(std::shared_ptr<Plugin> plugin, Interface **interfaceList)
{
    int (*getInstance)(Interface**) = (int(*)(Interface**))dlsym(plugin->GetHandler(), "get_instance");
    if (getInstance == nullptr) {
        ERROR("[PluginManager] dlsym error!\n");
        return -1;
    }
    int len = getInstance(interfaceList);
    DEBUG("[PluginManager] dl loaded! ");
    return len;
}

void PluginManager::SaveInstance(std::shared_ptr<Plugin> plugin, Interface *interfaceList, int len)
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
        DEBUG("[PluginManager] Instance: " << name.c_str());
        memoryStore.AddInstance(instance);
        plugin->AddInstance(instance);
    }
}

bool PluginManager::LoadInstance(std::shared_ptr<Plugin> plugin)
{
    int len = 0;
    DEBUG("plugin: " << plugin->GetName());
    Interface *interfaceList = nullptr;
    len = LoadDlInstance(plugin, &interfaceList);
    if (len < 0) {
        return false;
    }
    SaveInstance(plugin, interfaceList, len);
    return true;
}

ErrorCode PluginManager::LoadPlugin(const std::string &name)
{
    std::string plugin_path = GetPath() + "/" + name;
    if (!FileExist(plugin_path)) {
        return ErrorCode::LOAD_PLUGIN_FILE_NOT_EXIST;
    }
    if (!EndWith(name, ".so")) {
        return ErrorCode::LOAD_PLUGIN_FILE_IS_NOT_SO;
    }
    if (!CheckPermission(plugin_path, S_IRUSR | S_IRGRP)) {
        return ErrorCode::LOAD_PLUGIN_FILE_PERMISSION_DEFINED;
    }
    if (memoryStore.IsPluginExist(name)) {
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
    memoryStore.AddPlugin(name, plugin);
    return ErrorCode::OK;
}

std::string GenerateDot(MemoryStore &memoryStore, const std::vector<std::vector<std::string>> &query)
{
    std::string res;
    res += "digraph G {\n";
    res += "    rankdir = TB\n";
    res += "    ranksep = 1\n";
    std::unordered_map<std::string, std::unordered_set<std::string>> subGraph;
    for (auto &vec : query) {
        std::shared_ptr<Instance> instance = memoryStore.GetInstance(vec[0]);
        subGraph[instance->GetPluginName()].insert(vec[0]);
        if (vec.size() == 1) {
            continue;
        }
        if (memoryStore.IsInstanceExist(vec[1])) {
            instance = memoryStore.GetInstance(vec[1]);
            subGraph[instance->GetPluginName()].insert(vec[1]);
        } else {
            res += "    " + vec[1] + "[label=\"(missing)\\n" + vec[1] + "\", fontcolor=red];\n";
        }
        res += "    " + vec[0] + "->"  + vec[1] + ";\n";
    }
    int id = 0;
    for (auto &p : subGraph) {
        res += "    subgraph cluster_" + std::to_string(id) + " {\n";
        res += "        node [style=filled];\n";
        res += "        label = \"" + p.first + "\";\n";
        for (auto &i_name : p.second) {
            res += "        " + i_name + ";\n";
        }
        res += "    }\n";
        id++;
    }
    res += "}";
    return res;
}

ErrorCode PluginManager::QueryDependency(const std::string &name, std::string &res)
{
    if (!memoryStore.IsInstanceExist(name)) {
        return ErrorCode::QUERY_DEP_NOT_EXIST;
    }
    DEBUG("[PluginManager] query top : " << name);
    std::vector<std::vector<std::string>> query;
    memoryStore.QueryNodeDependency(name, query);
    res = GenerateDot(memoryStore, query);
    return ErrorCode::OK;
}

ErrorCode PluginManager::QueryAllDependencies(std::string &res)
{
    std::vector<std::vector<std::string>> query;
    memoryStore.QueryAllDependencies(query);
    DEBUG("[PluginManager] query size:" << query.size());
    res = GenerateDot(memoryStore, query);
    return ErrorCode::OK;
}

ErrorCode PluginManager::InstanceEnabled(const std::string &name)
{
    if (!memoryStore.IsInstanceExist(name)) {
        return ErrorCode::ENABLE_INSTANCE_NOT_LOAD;
    }
    std::shared_ptr<Instance> instance = memoryStore.GetInstance(name);
    if (!instance->GetState()) {
        return ErrorCode::ENABLE_INSTANCE_UNAVAILABLE;
    }
    if (instance->GetEnabled()) {
        return ErrorCode::ENABLE_INSTANCE_ALREADY_ENABLED;
    }
    std::shared_ptr<InstanceRunMessage> msg = std::make_shared<InstanceRunMessage>(RunType::ENABLED, instance);
    /* Send message to InstanceRunHandler. */
    instanceRunHandler->RecvQueuePush(msg);
    /* Wait for InstanceRunHandler to finsh this task. */
    msg->Wait();
    if (msg->GetInstance()->GetEnabled()) {
        return ErrorCode::OK;
    } else {
        return ErrorCode::ENABLE_INSTANCE_ENV;
    }
}

ErrorCode PluginManager::InstanceDisabled(const std::string &name)
{
    if (!memoryStore.IsInstanceExist(name)) {
        return ErrorCode::DISABLE_INSTANCE_NOT_LOAD;
    }
    std::shared_ptr<Instance> instance = memoryStore.GetInstance(name);
    if (!instance->GetState()) {
        return ErrorCode::DISABLE_INSTANCE_UNAVAILABLE;
    }
    if (!instance->GetEnabled()) {
        return ErrorCode::DISABLE_INSTANCE_ALREADY_DISABLED;
    }
    auto msg = std::make_shared<InstanceRunMessage>(RunType::DISABLED, instance);
    instanceRunHandler->RecvQueuePush(msg);
    msg->Wait();
    return ErrorCode::OK;
}

bool PluginManager::EndWith(const std::string &s, const std::string &ending)
{
    if (s.length() >= ending.length()) {
        return (s.compare(s.length() - ending.length(), ending.length(), ending) == 0);
    } else {
        return false;
    }
}

std::string PluginManager::GetPluginInDir(const std::string &path)
{
    std::string res;
    struct stat s = {};
    lstat(path.c_str(), &s);
    if (!S_ISDIR(s.st_mode)) {
        return res;
    }
    struct dirent *fileName = nullptr;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return res;
    }
    while ((fileName = readdir(dir)) != nullptr) {
        if (EndWith(std::string(fileName->d_name), ".so")) {
            res += std::string(fileName->d_name) + "\n";
        }
    }
    return res;
}

ErrorCode PluginManager::AddList(std::string &res)
{
    auto pluginList = config->GetPluginList();
    res += "Supported Packages:\n";
    for (auto &p : pluginList) {
        res += p.first + "\n";
    }
    res += "Installed Plugins:\n";
    res += GetPluginInDir(DEFAULT_PLUGIN_PATH);
    return ErrorCode::OK;
}

ErrorCode PluginManager::Download(const std::string &name, std::string &res)
{
    if (!config->IsPluginInfoExist(name)) {
        return ErrorCode::DOWNLOAD_NOT_FOUND;
    }
    res += config->GetPluginInfo(name).GetUrl();
    return ErrorCode::OK;
}

void PluginManager::EnablePlugin(const std::string &pluginName)
{
    std::shared_ptr<Plugin> plugin = memoryStore.GetPlugin(pluginName);
    for (size_t j = 0; j < plugin->GetInstanceLen(); ++j) {
        std::string name = plugin->GetInstance(j)->GetName();
        auto ret = InstanceEnabled(name);
        if (ret != ErrorCode::OK) {
            WARN("[PluginManager] " << name << " instance pre-enabled failed, because " <<
                ErrorText::GetErrorText(ret) << ".");
        } else {
            INFO("[PluginManager] " << name << " instance pre-enabled.");
        }
    }
}

void PluginManager::EnableInstance(const EnableItem &item)
{
    for (size_t j = 0; j < item.GetInstanceSize(); ++j) {
        std::string name = item.GetInstanceName(j);
        auto ret = InstanceEnabled(name);
        if (ret != ErrorCode::OK) {
            WARN("[PluginManager] " << name << " instance pre-enabled failed, because " <<
                ErrorText::GetErrorText(ret) << ".");
        } else {
            INFO("[PluginManager] " << name << " instance pre-enabled.");
        }
    }
}

void PluginManager::PreEnable()
{
    for (size_t i = 0; i < config->GetEnableListSize(); ++i) {
        EnableItem item = config->GetEnableList(i);
        std::string pluginName = item.GetName();
        if (!memoryStore.IsPluginExist(pluginName)) {
            WARN("[PluginManager] plugin " << pluginName << " cannot be enabled, because it does not exist.");
            continue;
        }
        if (item.GetEnabled()) {
            EnablePlugin(pluginName);
        } else {
            EnableInstance(item);
        }
    }
}

void PluginManager::PreLoadPlugin()
{
    std::string path = GetPath();
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (EndWith(name, ".so")) {
            auto ret = LoadPlugin(name);
            if (ret != ErrorCode::OK) {
                WARN("[PluginManager] " << name << " plugin preload failed, because " <<
                    ErrorText::GetErrorText(ret) << ".");
            } else {
                INFO("[PluginManager] " << name << " plugin loaded.");
            }
        }
    }
    closedir(dir);
}

void PluginManager::PreLoad()
{
    PreLoadPlugin();
    PreEnable();
}

void PluginManager::Exit()
{
    auto allPlugins = memoryStore.GetAllPlugins();
    auto msg = std::make_shared<InstanceRunMessage>(RunType::SHUTDOWN, nullptr);
    SendMsgToInstancRunHandler(msg);
    msg->Wait();
    for (auto plugin : allPlugins) {
        for (size_t i = 0; i < plugin->GetInstanceLen(); ++i) {
            auto instance = plugin->GetInstance(i);
            if (!instance->GetEnabled()) {
                continue;
            }
            instance->GetInterface()->disable();
            INFO("[PluginManager] " << instance->GetName() << " instance disabled.");
        }
    }
}

void PluginManager::ConstructLackDep(const std::vector<std::vector<std::string>> &query, std::string &res)
{
    std::vector<std::string> lack;
    size_t nodeCnt = 2;
    for (auto &item : query) {
        if (item.size() < nodeCnt) continue;
        if (!memoryStore.IsInstanceExist(item[1])) {
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

std::string PluginManager::InstanceDepCheck(const std::string &name)
{
    std::shared_ptr<Plugin> plugin = memoryStore.GetPlugin(name);
    std::string res;
    for (size_t i = 0; i < plugin->GetInstanceLen(); ++i) {
        std::string instanceName = plugin->GetInstance(i)->GetName();
        std::vector<std::vector<std::string>> query;
        memoryStore.QueryNodeDependency(instanceName, query);
        ConstructLackDep(query, res);
    }
    return res;
}

int PluginManager::Run()
{
    instanceRunHandler->Run();
    PreLoad();
    while (true) {
        Message msg;
        Message res;
        this->handlerMsg->WaitAndPop(msg);
        if (msg.getOpt() == Opt::SHUTDOWN) break;
        switch (msg.getOpt()) {
            case Opt::LOAD: {
                std::string pluginName = msg.GetPayload(0);
                ErrorCode retCode = LoadPlugin(pluginName);
                if (retCode == ErrorCode::OK) {
                    INFO("[PluginManager] " << pluginName << " plugin loaded.");
                    res.SetOpt(Opt::RESPONSE_OK);
                    std::string lackDep = InstanceDepCheck(pluginName);
                    if (!lackDep.empty()) {
                        INFO("[PluginManager] " << pluginName << " requires the following dependencies:\n" << lackDep);
                        res.AddPayload(lackDep);
                    }
                } else {
                    WARN("[PluginManager] " << pluginName << " " << ErrorText::GetErrorText(retCode)  << ".");
                    res.SetOpt(Opt::RESPONSE_ERROR);
                    res.AddPayload(ErrorText::GetErrorText(retCode));
                }
                break;
            }
            case Opt::REMOVE: {
                std::string name = msg.GetPayload(0);
                ErrorCode retCode = Remove(name);
                if (retCode == ErrorCode::OK) {
                    INFO("[PluginManager] " << name << " plugin removed.");
                    res.SetOpt(Opt::RESPONSE_OK);
                } else {
                    res.SetOpt(Opt::RESPONSE_ERROR);
                    res.AddPayload(ErrorText::GetErrorText(retCode));
                    INFO("[PluginManager] " << name << " " << ErrorText::GetErrorText(retCode) + ".");
                }
                break;
            }
            case Opt::QUERY_ALL: {
                std::string resText;
                ErrorCode retCode = QueryAllPlugins(resText);
                if (retCode == ErrorCode::OK) {
                    INFO("[PluginManager] query all plugins information.");
                    res.SetOpt(Opt::RESPONSE_OK);
                    res.AddPayload(resText);
                } else {
                    WARN("[PluginManager] query all plugins failed, because " << ErrorText::GetErrorText(retCode) <<
                        ".");
                    res.SetOpt(Opt::RESPONSE_ERROR);
                    res.AddPayload(ErrorText::GetErrorText(retCode));
                }
                break;
            }
            case Opt::QUERY: {
                std::string resText;
                std::string name = msg.GetPayload(0);
                ErrorCode retCode = QueryPlugin(name, resText);
                if (retCode == ErrorCode::OK) {
                    INFO("[PluginManager] " << name << " plugin query successfully.");
                    res.SetOpt(Opt::RESPONSE_OK);
                    res.AddPayload(resText);
                } else {
                    WARN("[PluginManager] " << name << " " << ErrorText::GetErrorText(retCode) + ".");
                    res.SetOpt(Opt::RESPONSE_ERROR);
                    res.AddPayload(ErrorText::GetErrorText(retCode));
                }
                break;
            }
            case Opt::QUERY_DEP: {
                std::string resText;
                std::string name = msg.GetPayload(0);
                ErrorCode retCode = QueryDependency(name, resText);
                if (retCode == ErrorCode::OK) {
                    INFO("[PluginManager] query " << name << " instance dependencies.");
                    res.SetOpt(Opt::RESPONSE_OK);
                    res.AddPayload(resText);
                } else {
                    WARN("[PluginManager] query  " << name  << " instance dependencies failed, because " <<
                        ErrorText::GetErrorText(retCode) << ".");
                    res.SetOpt(Opt::RESPONSE_ERROR);
                    res.AddPayload(ErrorText::GetErrorText(retCode));
                }
                break;
            }
            case Opt::QUERY_ALL_DEPS: {
                std::string resText;
                ErrorCode retCode = QueryAllDependencies(resText);
                if (retCode == ErrorCode::OK) {
                    INFO("[PluginManager] query all instances dependencies.");
                    res.SetOpt(Opt::RESPONSE_OK);
                    res.AddPayload(resText);
                } else {
                    WARN("[PluginManager] query all instances dependencies failed. because " <<
                        ErrorText::GetErrorText(retCode) << ".");
                    res.SetOpt(Opt::RESPONSE_ERROR);
                    res.AddPayload(ErrorText::GetErrorText(retCode));
                }
                break;
            }
            case Opt::ENABLED: {
                if (msg.GetPayloadLen() < 1) {
                    WARN("[PluginManager] enable opt need arg!");
                    res.AddPayload("enable opt need arg!");
                    break;
                }
                std::string instanceName = msg.GetPayload(0);
                ErrorCode retCode = InstanceEnabled(instanceName);
                if (retCode == ErrorCode::OK) {
                    INFO("[PluginManager] " << instanceName << " enabled successful.");
                    res.SetOpt(Opt::RESPONSE_OK);
                } else {
                    WARN("[PluginManager] " << instanceName << " enabled failed. because " <<
                        ErrorText::GetErrorText(retCode) + ".");
                    res.SetOpt(Opt::RESPONSE_ERROR);
                    res.AddPayload(ErrorText::GetErrorText(retCode));
                }
                break;
            }
            case Opt::DISABLED: {
                if (msg.GetPayloadLen() < 1) {
                    WARN("[PluginManager] disable opt need arg!");
                    res.AddPayload("disable opt need arg!");
                    break;
                }
                std::string instanceName = msg.GetPayload(0);
                ErrorCode retCode = InstanceDisabled(instanceName);
                if (retCode == ErrorCode::OK) {
                    INFO("[PluginManager] " << instanceName << " disabled successful.");
                    res.SetOpt(Opt::RESPONSE_OK);
                } else {
                    WARN("[PluginManager] " << instanceName << " " << ErrorText::GetErrorText(retCode) << ".");
                    res.SetOpt(Opt::RESPONSE_ERROR);
                    res.AddPayload(ErrorText::GetErrorText(retCode));
                }
                break;
            }
            case Opt::LIST: {
                std::string resText;
                ErrorCode retCode = AddList(resText);
                if (retCode == ErrorCode::OK) {
                    INFO("[PluginManager] query plugin_list.");
                    res.SetOpt(Opt::RESPONSE_OK);
                    res.AddPayload(resText);
                } else {
                    WARN("[PluginManager] query plugin_list failed, because " << ErrorText::GetErrorText(retCode) <<
                        ".");
                    res.SetOpt(Opt::RESPONSE_ERROR);
                    res.AddPayload(ErrorText::GetErrorText(retCode));
                }
                break;
            }
            case Opt::DOWNLOAD: {
                std::string resText;
                std::string name = msg.GetPayload(0);
                ErrorCode retCode = Download(name, resText);
                if (retCode == ErrorCode::OK) {
                    INFO("[PluginManager] download " << name << " from " << resText << ".");
                    res.SetOpt(Opt::RESPONSE_OK);
                    res.AddPayload(resText);
                } else {
                    WARN("[PluginManager] download " << name << " failed, because " <<
                        ErrorText::GetErrorText(retCode) + ".");
                    res.SetOpt(Opt::RESPONSE_ERROR);
                    res.AddPayload(ErrorText::GetErrorText(retCode));
                }
            }
            default:
                break;
        }
        if (msg.GetType() == MessageType::EXTERNAL)
            resMsg->Push(res);
    }
    Exit();
    return 0;
}
}

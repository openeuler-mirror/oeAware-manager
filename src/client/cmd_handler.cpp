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
#include "cmd_handler.h"
#include <fstream>
#include "oeaware/utils.h"

namespace oeaware {
std::unordered_set<std::string> LoadHandler::types = {"collector", "scenario", "tune"};

void LoadHandler::Check(const std::string &type)
{
    if (!types.count(type)) {
        ArgParse::GetInstance().ArgError("type '" + type + "' is not supported.");
    }
}

void LoadHandler::Handler(Message &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    msg.payload.emplace_back(arg);
    msg.opt = Opt::LOAD;
}

void LoadHandler::ResHandler(Message &msg)
{
    if (msg.opt == Opt::RESPONSE_OK) {
        std::cout << "Plugin loaded successfully.\n";
    } else {
        std::cout << "Plugin loaded failed, because "<< msg.payload[0] << ".\n";
    }
}

void QueryHandler::Handler(Message &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    if (arg.empty()) {
        msg.opt = Opt::QUERY_ALL;
    } else {
        msg.payload.emplace_back(arg);
        msg.opt = Opt::QUERY;
    }
}

void QueryHandler::PrintFormat()
{
    std::cout << "format:\n"
            "[plugin]\n"
            "\t[instance]([dependency status], [running status], [enable cnt])\n"
            "dependency status: available means satisfying dependency, otherwise unavailable.\n"
            "running status: running means that instance is running, otherwise close.\n"
            "enable cnt: number of instances enabled.\n";
}

void QueryHandler::ResHandler(Message &msg)
{
    if (msg.opt == Opt::RESPONSE_ERROR) {
        std::cout << "Plugin query failed, because " << msg.payload[0] <<".\n";
        return;
    }
    std::cout << "Show plugins and instances status.\n";
    std::cout << "------------------------------------------------------------\n";
    for (auto &info : msg.payload) {
        std::cout << info;
    }
    std::cout << "------------------------------------------------------------\n";
    PrintFormat();
}

void RemoveHandler::Handler(Message &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    msg.payload.emplace_back(arg);
    msg.opt = Opt::REMOVE;
}

void RemoveHandler::ResHandler(Message &msg)
{
    if (msg.opt == Opt::RESPONSE_OK) {
        std::cout << "Plugin remove successfully.\n";
    } else {
        std::cout << "Plugin remove failed, because " << msg.payload[0] << ".\n";
    }
}

void GeneratePngFromDot(const std::string &dotFile, const std::string &pngFile)
{
    std::string command = "dot -Tpng " + dotFile + " -o " + pngFile;
    std::system(command.c_str());
}

void write_to_file(const std::string &file_name, const std::string &text)
{
    std::ofstream out(file_name);
    if (!out.is_open()) return;
    out << text;
    out.close();
}

void QueryTopHandler::Handler(Message &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    if (arg.empty()) {
        msg.opt = Opt::QUERY_ALL_SUB_GRAPH;
    } else {
        msg.payload.emplace_back(arg);
        msg.opt = Opt::QUERY_SUB_GRAPH;
    }
}

void QueryTopHandler::ResHandler(Message &msg)
{
    if (msg.opt == Opt::RESPONSE_ERROR) {
        std::cout << "Query instance dependencies failed, because "<< msg.payload[0] << ".\n";
        return;
    }
    std::string text = msg.payload[0];
    write_to_file("dep.dot", text);
    GeneratePngFromDot("dep.dot", "dep.png");
    std::cout << "Generate dependencies graph dep.png.\n";
}

void EnabledHandler::Handler(Message &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    msg.payload.emplace_back(arg);
    std::string param = "";
    bool first = true;
    for (auto &p : ArgParse::GetInstance().GetEnableParams()) {
        if (!first) param += ",";
        param += p.first;
        param += ":";
        param += p.second;
        first = false;
    }
    msg.payload.emplace_back(param);
    msg.opt = Opt::ENABLED;
}

void EnabledHandler::ResHandler(Message &msg)
{
    if (msg.opt == Opt::RESPONSE_OK) {
        std::cout << "Instance enabled successfully.\n";
    } else {
        std::cout << "Instance enabled failed, because "<< msg.payload[0] << "\n";
    }
}

void DisabledHandler::Handler(Message &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    msg.payload.emplace_back(arg);
    msg.opt = Opt::DISABLED;
}

void DisabledHandler::ResHandler(Message &msg)
{
    if (msg.opt == Opt::RESPONSE_OK) {
        std::cout << "Instance disabled successfully.\n";
    } else {
        std::cout << "Instance disabled failed, because "<< msg.payload[0] << ".\n";
    }
}

void ListHandler::Handler(Message &msg)
{
    msg.opt = Opt::LIST;
}

void ListHandler::ResHandler(Message &msg)
{
    if (msg.opt == Opt::RESPONSE_ERROR) {
        std::cerr << "Query list failed, because "<< msg.payload[0] << ".\n";
        return;
    }
    std::cout << "Plugin list as follows.\n";
    std::cout << "------------------------------------------------------------\n";
    for (auto &info : msg.payload) {
        std::cout << info;
    }
    std::cout << "------------------------------------------------------------\n";
}

void InfoCmdHandler::Handler(Message &msg)
{
    msg.opt = Opt::INFO;
}

void InfoCmdHandler::ResHandler(Message &msg)
{
    if (msg.opt == Opt::RESPONSE_ERROR) {
        std::cerr << "Query list failed, because "<< msg.payload[0] << ".\n";
        return;
    }
    std::cout << "The tuned instance plugin table is as follows.\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Use oeawarectl -e [instance] to enable.\n";
    std::cout << "    oeawarectl -d [instance] to disable.\n";
    std::cout << "------------------------------------------------------------\n";
    for (auto &info : msg.payload) {
        std::cout << info;
    }
}

void InstallHandler::Handler(Message &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    msg.opt = Opt::DOWNLOAD;
    msg.payload.emplace_back(arg);
}

void InstallHandler::ResHandler(Message &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    if (msg.opt == Opt::RESPONSE_ERROR) {
        std::cout << "Download failed, because " << msg.payload[0] <<": " << arg.c_str() << '\n';
        return;
    }
    std::string path = arg;
    std::string url = msg.payload[0];
    if (!Download(url, path)) {
        std::cout << "Download failed, please check url or your network.\n";
        return;
    }
    std::string command = "rpm -ivh " + path;
    std::string rm = "rm -f " + path;
    system(command.c_str());
    system(rm.c_str());
}

void StartHandler::Handler(Message &msg)
{
    (void)msg;
    system("/bin/oeaware /etc/oeaware/config.yaml &> /dev/null &");
    printf("oeaware started.\n");
    exit(0);
}

void StartHandler::ResHandler(Message &msg)
{
    (void)msg;
}

void StopHandler::Handler(Message &msg)
{
    (void)msg;
    msg.opt = Opt::SHUTDOWN;
}

void StopHandler::ResHandler(Message &msg)
{
    (void)msg;
    printf("oeaware stopped.\n");
}

void ReloadConfHandler::Handler(Message &msg)
{
    msg.opt = Opt::RELOAD_CONF;
}

void ReloadConfHandler::ResHandler(Message &msg)
{
    if (msg.opt == Opt::RESPONSE_OK) {
        std::cout << "Reload config successfully." + msg.payload[0] + "\n";
    } else {
        std::cout << "Reload config failed, because " << msg.payload[0] << ".\n";
    }
}

}
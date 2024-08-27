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
#include "utils.h"

namespace oeaware {
std::unordered_set<std::string> LoadHandler::types = {"collector", "scenario", "tune"};

void LoadHandler::Check(const std::string &type)
{
    if (!types.count(type)) {
        ArgParse::GetInstance().ArgError("type '" + type + "' is not supported.");
    }
}

void LoadHandler::Handler(Msg &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    msg.AddPayload(arg);
    msg.SetOpt(Opt::LOAD);
}

void LoadHandler::ResHandler(Msg &msg)
{
    if (msg.GetOpt() == Opt::RESPONSE_OK) {
        std::cout << "Plugin loaded successfully.";
        if (msg.PayloadSize()) {
            std::cout << "But plugin requires the following dependencies to run.\n";
            for (int i = 0; i < msg.PayloadSize(); ++i) {
                std::cout << msg.Payload(i) << '\n';
            }
        } else {
            std::cout << '\n';
        }
    } else {
        std::cout << "Plugin loaded failed, because "<< msg.Payload(0) << ".\n";
    }
}

void QueryHandler::Handler(Msg &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    if (arg.empty()) {
        msg.SetOpt(Opt::QUERY_ALL);
    } else {
        msg.AddPayload(arg);
        msg.SetOpt(Opt::QUERY);
    }
}

void QueryHandler::PrintFormat()
{
    std::cout << "format:\n"
            "[plugin]\n"
            "\t[instance]([dependency status], [running status])\n"
            "dependency status: available means satisfying dependency, otherwise unavailable.\n"
            "running status: running means that instance is running, otherwise close.\n";
}

void QueryHandler::ResHandler(Msg &msg)
{
    if (msg.GetOpt() == Opt::RESPONSE_ERROR) {
        std::cout << "Plugin query failed, because " << msg.Payload(0).c_str() <<".\n";
        return;
    }
    int len = msg.PayloadSize();
    std::cout << "Show plugins and instances status.\n";
    std::cout << "------------------------------------------------------------\n";
    for (int i = 0; i < len; ++i) {
        std::cout << msg.Payload(i).c_str();
    }
    std::cout << "------------------------------------------------------------\n";
    PrintFormat();
}

void RemoveHandler::Handler(Msg &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    msg.AddPayload(arg);
    msg.SetOpt(Opt::REMOVE);
}

void RemoveHandler::ResHandler(Msg &msg)
{
    if (msg.GetOpt() == Opt::RESPONSE_OK) {
        std::cout << "Plugin remove successfully.\n";
    } else {
        std::cout << "Plugin remove failed, because " << msg.Payload(0) << ".\n";
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

void QueryTopHandler::Handler(Msg &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    if (arg.empty()) {
        msg.SetOpt(Opt::QUERY_ALL_DEPS);
    } else {
        msg.AddPayload(arg);
        msg.SetOpt(Opt::QUERY_DEP);
    }
}

void QueryTopHandler::ResHandler(Msg &msg)
{
    if (msg.GetOpt() == Opt::RESPONSE_ERROR) {
        std::cout << "Query instance dependencies failed, because "<< msg.Payload(0) << ".\n";
        return;
    }
    std::string text = msg.Payload(0);
    write_to_file("dep.dot", text);
    GeneratePngFromDot("dep.dot", "dep.png");
    std::cout << "Generate dependencies graph dep.png.\n";
}

void EnabledHandler::Handler(Msg &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    msg.AddPayload(arg);
    msg.SetOpt(Opt::ENABLED);
}

void EnabledHandler::ResHandler(Msg &msg)
{
    if (msg.GetOpt() == Opt::RESPONSE_OK) {
        std::cout << "Instance enabled successfully.\n";
    } else {
        std::cout << "Instance enabled failed, because "<< msg.Payload(0) << ".\n";
    }
}

void DisabledHandler::Handler(Msg &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    msg.AddPayload(arg);
    msg.SetOpt(Opt::DISABLED);
}

void DisabledHandler::ResHandler(Msg &msg)
{
    if (msg.GetOpt() == Opt::RESPONSE_OK) {
        std::cout << "Instance disabled successfully.\n";
    } else {
        std::cout << "Instance disabled failed, because "<< msg.Payload(0) << ".\n";
    }
}

void ListHandler::Handler(Msg &msg)
{
    msg.SetOpt(Opt::LIST);
}

void ListHandler::ResHandler(Msg &msg)
{
    if (msg.GetOpt() == Opt::RESPONSE_ERROR) {
        std::cerr << "Query list failed, because "<< msg.Payload(0) << ".\n";
        return;
    }
    std::cout << "Plugin list as follows.\n";
    std::cout << "------------------------------------------------------------\n";
    for (int i = 0; i < msg.PayloadSize(); ++i) {
        std::cout << msg.Payload(i);
    }
    std::cout << "------------------------------------------------------------\n";
}

void InstallHandler::Handler(Msg &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    msg.SetOpt(Opt::DOWNLOAD);
    msg.AddPayload(arg);
}

void InstallHandler::ResHandler(Msg &msg)
{
    std::string arg = ArgParse::GetInstance().GetArg();
    if (msg.GetOpt() == Opt::RESPONSE_ERROR) {
        std::cout << "Download failed, because " << msg.Payload(0) <<": " << arg.c_str() << '\n';
        return;
    }
    std::string path = arg;
    std::string url = msg.Payload(0);
    if (!Download(url, path)) {
        std::cout << "Download failed, please check url or your network.\n";
        return;
    }
    std::string command = "rpm -ivh " + path;
    std::string rm = "rm -f " + path;
    system(command.c_str());
    system(rm.c_str());
}
}

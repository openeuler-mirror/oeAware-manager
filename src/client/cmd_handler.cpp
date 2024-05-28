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
#include "utils.h"
#include <fstream>

std::unordered_set<std::string> LoadHandler::types = {"collector", "scenario", "tune"};

void LoadHandler::check(const std::string &type) {
    if (!types.count(type)) {
        ArgParse::arg_error("type '" + type + "' is not supported.");
    }
}

void LoadHandler::handler(Msg &msg) {
    std::string arg = ArgParse::get_arg();
    msg.add_payload(arg);
    msg.set_opt(Opt::LOAD);
}

void LoadHandler::res_handler(Msg &msg) {
    if (msg.get_opt() == Opt::RESPONSE_OK) {
        std::cout << "Plugin loaded successfully.";
        if (msg.payload_size()) {
            std::cout << "But plugin requires the following dependencies to run.\n";
            for (int i = 0; i < msg.payload_size(); ++i) {
                std::cout << msg.payload(i) << '\n';
            }
        } else {
            std::cout << '\n';
        }
    } else {
        std::cout << "Plugin loaded failed, because "<< msg.payload(0) << ".\n";
    }
    
}

void QueryHandler::handler(Msg &msg) {
    std::string arg = ArgParse::get_arg();
    if (arg.empty()) {
        msg.set_opt(Opt::QUERY_ALL);
    } else {
        msg.add_payload(arg);
        msg.set_opt(Opt::QUERY);
    }
}

void QueryHandler::print_format() {
    std::cout << "format:\n"
            "[plugin]\n"
            "\t[instance]([dependency status], [running status])\n"
            "dependency status: available means satisfying dependency, otherwise unavailable.\n"
            "running status: running means that instance is running, otherwise close.\n";
}

void QueryHandler::res_handler(Msg &msg) {
    if (msg.get_opt() == Opt::RESPONSE_ERROR) {
        std::cout << "Plugin query failed, because " << msg.payload(0).c_str() <<".\n";
        return;
    } 
    int len = msg.payload_size();
    std::cout << "Show plugins and instances status.\n";
    std::cout << "------------------------------------------------------------\n";
    for (int i = 0; i < len; ++i) {
        std::cout << msg.payload(i).c_str();
    }
    std::cout << "------------------------------------------------------------\n";
    print_format();
}

void RemoveHandler::handler(Msg &msg) {
    std::string arg = ArgParse::get_arg();
    msg.add_payload(arg);
    msg.set_opt(Opt::REMOVE);
}

void RemoveHandler::res_handler(Msg &msg) {
    if (msg.get_opt() == Opt::RESPONSE_OK) {
        std::cout << "Plugin remove successfully.\n";
    } else {
        std::cout << "Plugin remove failed, because " << msg.payload(0) << ".\n";
    }
}

void generate_png_from_dot(const std::string &dot_file, const std::string &png_file) {
    std::string command = "dot -Tpng " + dot_file + " -o " + png_file;
    std::system(command.c_str());
}

void write_to_file(const std::string &file_name, const std::string &text) {
    std::ofstream out(file_name);
    if (!out.is_open()) return;
    out << text;
    out.close(); 
}

void QueryTopHandler::handler(Msg &msg) {
    std::string arg = ArgParse::get_arg();
    if (arg.empty()) { 
        msg.set_opt(Opt::QUERY_ALL_DEPS);
    } else {
        msg.add_payload(arg);
        msg.set_opt(Opt::QUERY_DEP);
    }
}

void QueryTopHandler::res_handler(Msg &msg) {
    if (msg.get_opt() == Opt::RESPONSE_ERROR) {
        std::cout << "Query instance dependencies failed, because "<< msg.payload(0) << ".\n";
        return;
    }
    std::string text = msg.payload(0);
    write_to_file("dep.dot", text);
    generate_png_from_dot("dep.dot", "dep.png");
    std::cout << "Generate dependencies graph dep.png.\n";
}

void EnabledHandler::handler(Msg &msg) {
    std::string arg = ArgParse::get_arg();
    msg.add_payload(arg);
    msg.set_opt(Opt::ENABLED);
}

void EnabledHandler::res_handler(Msg &msg) {
    if (msg.get_opt() == Opt::RESPONSE_OK) {
        std::cout << "Instance enabled successfully.\n";
    } else {
        std::cout << "Instance enabled failed, because "<< msg.payload(0) << ".\n";
    }
}

void DisabledHandler::handler(Msg &msg) {
    std::string arg = ArgParse::get_arg();
    msg.add_payload(arg);
    msg.set_opt(Opt::DISABLED);
}

void DisabledHandler::res_handler(Msg &msg) {
    if (msg.get_opt() == Opt::RESPONSE_OK) {
        std::cout << "Instance disabled successfully.\n";
    } else {
        std::cout << "Instance disabled failed, because "<< msg.payload(0) << ".\n";
    }
}

void ListHandler::handler(Msg &msg) {
    msg.set_opt(Opt::LIST);
}

void ListHandler::res_handler(Msg &msg) {
    if (msg.get_opt() == Opt::RESPONSE_ERROR) {
        std::cerr << "Query list failed, because "<< msg.payload(0) << ".\n";
        return;
    }
    std::cout << "Plugin list as follows.\n";
    std::cout << "------------------------------------------------------------\n";
    for (int i = 0; i < msg.payload_size(); ++i) {
        std::cout << msg.payload(i);
    }
    std::cout << "------------------------------------------------------------\n";
}

void InstallHandler::handler(Msg &msg) {
    std::string arg = ArgParse::get_arg();
    msg.set_opt(Opt::DOWNLOAD);
    msg.add_payload(arg);
}

void InstallHandler::res_handler(Msg &msg) {
    std::string arg = ArgParse::get_arg();
    if (msg.get_opt() == Opt::RESPONSE_ERROR) {
        std::cout << "Download failed, because " << msg.payload(0) <<": " << arg.c_str() << '\n';
        return;
    }
    std::string path = arg;
    std::string url = msg.payload(0);
    if (!download(url, path)) {
        std::cout << "Download failed, please check url or your network.\n";
        return;
    }
    std::string command = "rpm -ivh " + path;
    std::string rm = "rm -f " + path;
    system(command.c_str());
    system(rm.c_str());
}

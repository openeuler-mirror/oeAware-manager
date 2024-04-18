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

CmdHandler* get_cmd_handler(int cmd) {
    switch (cmd) {
        case 'l':
            return new LoadHandler();
        case 'q':
            return new QueryHandler();
        case 'r':
            return new RemoveHandler();
        case 'Q':
            return new QueryTopHandler();
        case 'e':
            return new EnabledHandler();
        case 'd':
            return new DisabledHandler();
        case 'L':
            return new ListHandler();
        case 'D':
            return new DownloadHandler();
        default:
            return nullptr;
    }
    return nullptr;
}

void set_type(char *_type) {
    type = _type;
}

void set_arg(char *_arg) {
    arg = std::string(_arg);
}

void LoadHandler::handler(Msg &msg) {
    if (type.empty()) {
        printf("plugin type needed!\n");
        exit(EXIT_FAILURE);
    }
    msg.add_payload(arg);
    msg.add_payload(type);
    msg.set_opt(Opt::LOAD);
}

void LoadHandler::res_handler(Msg &msg) {
    for (int i = 0; i < msg.payload_size(); ++i) {
        printf("%s\n", msg.payload(i).c_str());
    }
}

void QueryHandler::handler(Msg &msg) {
    if (arg.empty()) {
        msg.set_opt(Opt::QUERY_ALL);
    } else {
        msg.add_payload(arg);
        msg.set_opt(Opt::QUERY);
    }
}

void QueryHandler::res_handler(Msg &msg) {
    int len = msg.payload_size();
    if (len == 0) {
        printf("no plugins loaded!\n");
        return;
    }
    for (int i = 0; i < len; ++i) {
        printf("%s\n", msg.payload(i).c_str());
    }
}

void RemoveHandler::handler(Msg &msg) {
    msg.add_payload(arg);
    msg.set_opt(Opt::REMOVE);
}

void RemoveHandler::res_handler(Msg &msg) {
    printf("%s\n", msg.payload(0).c_str());
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
    if (arg.empty()) { 
        msg.set_opt(Opt::QUERY_ALL_TOP);
    } else {
        msg.add_payload(arg);
        msg.set_opt(Opt::QUERY_TOP);
    }
}

void QueryTopHandler::res_handler(Msg &msg) {
    std::string text;
    for(int i = 0; i < msg.payload_size(); ++i) {
        text += msg.payload(i).c_str();
    }
    write_to_file("dep.dot", text);
    generate_png_from_dot("dep.dot", "dep.png");
}

void EnabledHandler::handler(Msg &msg) {
    msg.add_payload(arg);
    msg.set_opt(Opt::ENABLED);
}

void EnabledHandler::res_handler(Msg &msg) {
    printf("%s\n", msg.payload(0).c_str());
}

void DisabledHandler::handler(Msg &msg) {
    msg.add_payload(arg);
    msg.set_opt(Opt::DISABLED);
}

void DisabledHandler::res_handler(Msg &msg) {
    printf("%s\n", msg.payload(0).c_str());
}

void ListHandler::handler(Msg &msg) {
    msg.set_opt(Opt::LIST);
}

void ListHandler::res_handler(Msg &msg) {
    for (int i = 0; i < msg.payload_size(); ++i) {
        printf("%s", msg.payload(i).c_str());
    }
}

void DownloadHandler::handler(Msg &msg) {
    msg.set_opt(Opt::DOWNLOAD);
    msg.add_payload(arg);
}

void DownloadHandler::res_handler(Msg &msg) {
    std::string path = arg;
    std::string url = msg.payload(0);
    download(url, path);
    std::string command = "rpm -ivh " + path;
    std::string rm = "rm -f " + path;
    system(command.c_str());
    system(rm.c_str());
}

void print_help() {
    printf("oeAware-client [options]...\n"
           "  options\n"
           "    -l|--load [plugin]      load plugin and need plugin type.\n"
           "    -t|--type [plugin_type] assign plugin type.\n"
           "    -r|--remove [plugin]    remove plugin from system.\n"
           "    -e|--enable [instance]  enable the plugin instance.\n"
           "    -d|--disable [instance] disable the plugin instance.\n"
           "    -q                      query all plugins information.\n"
           "    --query [plugin]        query the plugin information.\n"
           "    -Q                      query all instances dependencies.\n"
           "    --query-dep [instance]  query the instance dependency.\n"
           "    --list                  the list of supported plugins.\n"
           "    --download [plugin]     download plugin from the list.\n"
           );
}
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
#include "arg_parse.h"
#include <iostream>
#include <getopt.h>

const std::string ArgParse::OPT_STRING = "Qqd:t:l:r:e:i:";
const struct option ArgParse::long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"load", required_argument, NULL, 'l'},
    {"type", required_argument, NULL, 't'},
    {"remove", required_argument, NULL, 'r'}, 
    {"query", required_argument, NULL, 'q'},
    {"query-dep", required_argument, NULL, 'Q'},
    {"enable", required_argument, NULL, 'e'},
    {"disable", required_argument, NULL, 'd'},
    {"list", no_argument, NULL, 'L'},
    {"install", required_argument, NULL, 'i'},
    {0, 0, 0, 0},
};

void ArgParse::arg_error(const std::string &msg) {
    std::cerr << "oeawarectl: " << msg << "\n";
    print_help();
    exit(EXIT_FAILURE);
}

void ArgParse::set_type(char *_type) {
    type = _type;
}

void ArgParse::set_arg(char *_arg) {
    arg = std::string(_arg);
}

void ArgParse::print_help() {
    std::cout << "usage: oeawarectl [options]...\n"
           "  options\n"
           "    -l|--load [plugin]      load plugin and need plugin type.\n"
           "    -t|--type [plugin_type] assign plugin type. there are three types:\n"
           "                            collector: collection plugin.\n"
           "                            scenario: awareness plugin.\n"
           "                            tune: tune plugin.\n"
           "    -r|--remove [plugin]    remove plugin from system.\n"
           "    -e|--enable [instance]  enable the plugin instance.\n"
           "    -d|--disable [instance] disable the plugin instance.\n"
           "    -q                      query all plugins information.\n"
           "    --query [plugin]        query the plugin information.\n"
           "    -Q                      query all instances dependencies.\n"
           "    --query-dep [instance]  query the instance dependency.\n"
           "    --list                  the list of supported plugins.\n"
           "    -i|--install [plugin]   install plugin from the list.\n"
           "    --help                  show this help message.\n";
}

void ArgParse::init_opts() {
    opts.insert('l');
    opts.insert('r');
    opts.insert('q');
    opts.insert('Q');
    opts.insert('e');
    opts.insert('d');
    opts.insert('i');
    opts.insert('t');
}

int ArgParse::init(int argc, char *argv[]) {
    int cmd = -1;
    int opt;
    bool help = false;
    init_opts();
    opterr = 0;
    while((opt = getopt_long(argc, argv, OPT_STRING.c_str(), long_options, nullptr)) != -1) {
        std::string full_opt;
        switch (opt) {
            case 't':
                set_type(optarg);
                break;
            case 'h':
                help = true;
                break;
            case '?': {
                std::string opt_string;
                if (optind == argc) {
                    opt_string += argv[optind - 1];    
                } else {
                    opt_string += argv[optind];
                }
                if (!opts.count(optopt)) {
                    arg_error("unknown option '" + opt_string  + "'.");
                } else{
                    arg_error("option " + opt_string  + " requires an argument.");
                }
                break;
            }
            default: {
                if (opt == 'l' || opt == 'r' || opt == 'q' || opt == 'Q' || opt == 'e' || opt == 'd'  || opt == 'L' || opt == 'i') {
                    if (cmd != -1) {
                        arg_error("invalid option.");
                        return -1;
                    }
                    cmd = opt;
                    if (optarg) {
                        set_arg(optarg);
                    }
                } 
                break;
            }
                
        }
    }
    if (cmd == 'l' && type.empty()) {
        arg_error("missing arguments.");
    }
    if (help) {
        print_help();
        exit(EXIT_SUCCESS);
    }
    if (cmd < 0) {
        arg_error("option error.");
    }
    return cmd;
}

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
#include <cstring>

namespace oeaware {
const std::string ArgParse::optString = "Qqd:l:r:e:i:";


void ArgParse::ArgError(const std::string &msg)
{
    std::cerr << "oeawarectl: " << msg << "\n";
    PrintHelp();
    exit(EXIT_FAILURE);
}

void ArgParse::SetArg(const std::string &newArg)
{
    arg = newArg;
}

void ArgParse::PrintHelp()
{
    std::cout << "usage: oeawarectl [options]...\n"
           "  options\n"
           "    -l|--load [plugin]      load plugin and need plugin type.\n"
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

void ArgParse::InitOpts()
{
    opts.insert('l');
    opts.insert('r');
    opts.insert('q');
    opts.insert('Q');
    opts.insert('e');
    opts.insert('d');
    opts.insert('i');
    longOptions.emplace_back(Option{"help", no_argument, NULL, 'h'});
    longOptions.emplace_back(Option{"load", required_argument, NULL, 'l'});
    longOptions.emplace_back(Option{"remove", required_argument, NULL, 'r'});
    longOptions.emplace_back(Option{"query", required_argument, NULL, 'q'});
    longOptions.emplace_back(Option{"query-dep", required_argument, NULL, 'Q'});
    longOptions.emplace_back(Option{"enable", required_argument, NULL, 'e'});
    longOptions.emplace_back(Option{"disable", required_argument, NULL, 'd'});
    longOptions.emplace_back(Option{"install", required_argument, NULL, 'i'});
    longOptions.emplace_back(Option{"list", no_argument, NULL, 'L'});
}

int ArgParse::InitCmd(int &cmd, int opt)
{
    if (opt == 'l' || opt == 'r' || opt == 'q' || opt == 'Q' || opt == 'e' || opt == 'd'  || opt == 'L' || opt == 'i') {
        if (cmd != -1) {
            ArgError("invalid option.");
            return -1;
        }
        cmd = opt;
        if (optarg) {
            SetArg(optarg);
        }
    }
    return 0;
}

int ArgParse::Init(int argc, char *argv[])
{
    constexpr int START_OR_STOP = 2;
    if (argc == START_OR_STOP) {
        if (strcmp(argv[1], "start") == 0) {
            return START;
        } else if (strcmp(argv[1], "stop") == 0) {
            return STOP;
        }
    }
    int cmd = -1;
    int opt;
    bool help = false;
    InitOpts();
    opterr = 0;
    while ((opt = getopt_long(argc, argv, optString.c_str(), longOptions.data(), nullptr)) != -1) {
        switch (opt) {
            case 'h':
                help = true;
                break;
            case '?': {
                std::string optString;
                if (optind == argc) {
                    optString += argv[optind - 1];
                } else {
                    optString += argv[optind];
                }
                if (!opts.count(optopt)) {
                    ArgError("unknown option '" + optString  + "'.");
                } else {
                    ArgError("option " + optString  + " requires an argument.");
                }
                break;
            }
            default: {
                InitCmd(cmd, opt);
                break;
            }
        }
    }
    if (help) {
        PrintHelp();
        exit(EXIT_SUCCESS);
    }
    if (cmd < 0) {
        ArgError("option error.");
    }
    return cmd;
}
}

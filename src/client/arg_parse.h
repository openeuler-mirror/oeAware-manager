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
#ifndef CLIENT_ARG_PARSE_H
#define CLIENT_ARG_PARSE_H
#include <string>
#include <unordered_set>

class ArgParse {
public:
    static void arg_error(const std::string &msg);
    static void print_help();
    static int init(int argc, char *argv[]);
    static void init_opts();
    static void set_type(char* _type);
    static void set_arg(char* _arg);
    static std::string get_type() {
        return type;
    }
    static std::string get_arg() {
        return arg;
    }
private:
    static std::string arg;
    static std::string type;
    static std::unordered_set<char> opts;
    static const std::string OPT_STRING;
    static const int MAX_OPT_SIZE = 20;
    static const struct option long_options[MAX_OPT_SIZE];
};

#endif // !CLIENT_ARG_PARSE_H

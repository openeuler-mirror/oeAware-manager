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
#include <vector>
#include <getopt.h>

namespace oeaware {
const int START = 0;
const int STOP = 1;
class ArgParse {
public:
    using Option = struct option;
    ArgParse(const ArgParse&) = delete;
    ArgParse& operator=(const ArgParse&) = delete;
    static ArgParse& GetInstance()
    {
        static ArgParse argParse;
        return argParse;
    }
    void ArgError(const std::string &msg);
    void PrintHelp();
    int Init(int argc, char *argv[]);
    std::string GetArg()
    {
        return arg;
    }
private:
    ArgParse() { }
    void InitOpts();
    int InitCmd(int &cmd, int opt);
    void SetArg(const std::string &newArg);
private:
    std::string arg;
    std::unordered_set<char> opts;
    static const std::string optString;
    std::vector<Option> longOptions;
};
}

#endif // !CLIENT_ARG_PARSE_H

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
           "    --query-top [instance]  query the instance dependency.\n");
}
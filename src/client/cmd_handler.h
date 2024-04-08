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
#ifndef CLIENT_CMD_HANDLER_H
#define CLIENT_CMD_HANDLER_H

#include "message.h"

static std::string type;
static std::string arg;

class CmdHandler {
public:
    CmdHandler(){}
    virtual bool handler(server::Msg *msg) = 0;
    virtual bool res_handler(server::Msg *msg) = 0;
};

class LoadHandler : public CmdHandler {
public:
    bool handler(server::Msg *msg) override {
        if (type.empty()) {
            printf("plugin type needed!\n");
            return false;
        }
        msg->add_payload(arg);
        msg->add_payload(type);
        msg->set_opt(server::Opt::LOAD);
        return true;
    }

    bool res_handler(server::Msg *res) override {
        for (int i = 0; i < res->payload_size(); ++i) {
            printf("%s\n", res->payload(i).c_str());
        }
        return true;
    }
};

class QueryHandler : public CmdHandler {
public:
    bool handler(server::Msg *msg) override {
        if (arg.empty()) {
            msg->set_opt(server::Opt::QUERY_ALL);
        } else {
            msg->add_payload(arg);
            msg->set_opt(server::Opt::QUERY);
        }
        return true;
    }

    bool res_handler(server::Msg *res) override {
        int len = res->payload_size();
        if (len == 0) {
            printf("no plugins loaded!\n");
            return true;
        }
        for (int i = 0; i < len; ++i) {
            printf("%s\n", res->payload(i).c_str());
        }
        return true;
    }
};

class RemoveHandler : public CmdHandler {
public:
    bool handler(server::Msg *msg) override {
        msg->add_payload(arg);
        msg->set_opt(server::Opt::REMOVE);
        return true;
    }

    bool res_handler(server::Msg *res) override {
        printf("%s\n", res->payload(0).c_str());
        return true;
    }
};


class QueryTopHandler : public CmdHandler {
public:
    bool handler(server::Msg *msg) override {
        if (arg.empty()) { 
            msg->set_opt(server::Opt::QUERY_ALL_TOP);
        } else {
            msg->add_payload(arg);
            msg->set_opt(server::Opt::QUERY_TOP);
        }
        return true;
    }

    bool res_handler(server::Msg *res) override {
        for (int i = 0; i < res->payload_size(); ++i) {
            printf("%s\n", res->payload(i).c_str());
        }
        return true;
    }
};

class EnabledHandler : public CmdHandler {
public:
    bool handler(server::Msg *msg) override {
        msg->add_payload(arg);
        msg->set_opt(server::Opt::ENABLED);
        return true;
    }

    bool res_handler(server::Msg *res) override {
        printf("%s\n", res->payload(0).c_str());
        return true;
    }
};

class DisabledHandler : public CmdHandler {
public:
    bool handler(server::Msg *msg) override {
        msg->add_payload(arg);
        msg->set_opt(server::Opt::DISABLED);
        return true;
    }

    bool res_handler(server::Msg *res) override {
        printf("%s\n", res->payload(0).c_str());
        return true;
    }
};

class ListHandler : public CmdHandler {
public:
    bool handler(server::Msg *msg) override {
        msg->set_opt(server::Opt::LIST);
        return true;
    }

    bool res_handler(server::Msg *res) override {
        for (int i = 0; i < res->payload_size(); ++i) {
            printf("%s\n", res->payload(i).c_str());
        }
        return true;
    }
};


CmdHandler* get_cmd_handler(int cmd);

void set_type(char* _type);
void set_arg(char* _arg);
void print_help();
#endif // !CLIENT_CMD_HANDLER_H
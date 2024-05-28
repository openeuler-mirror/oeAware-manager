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
#include "message_protocol.h"
#include "arg_parse.h"

class CmdHandler {
public:
    virtual void handler(Msg &msg) = 0;
    virtual void res_handler(Msg &msg) = 0;

    static ArgParse &arg_parse;
};

class LoadHandler : public CmdHandler {
public:
    void handler(Msg &msg) override;
    void res_handler(Msg &msg) override;
private:
    void check(const std::string &type);
    static std::unordered_set<std::string> types; 
};

class QueryHandler : public CmdHandler {
public:
    void handler(Msg &msg) override;
    void res_handler(Msg &msg) override;
private:
    void print_format();
};

class RemoveHandler : public CmdHandler {
public:
    void handler(Msg &msg) override;
    void res_handler(Msg &msg) override;
};


class QueryTopHandler : public CmdHandler {
public:
    void handler(Msg &msg) override;
    void res_handler(Msg &msg) override;
};

class EnabledHandler : public CmdHandler {
public:
    void handler(Msg &msg) override;
    void res_handler(Msg &msg) override;
};

class DisabledHandler : public CmdHandler {
public:
    void handler(Msg &msg) override;
    void res_handler(Msg &msg) override;
};

class ListHandler : public CmdHandler {
public:
    void handler(Msg &msg) override;
    void res_handler(Msg &msg) override;
};

class InstallHandler : public CmdHandler {
public:
    void handler(Msg &msg) override;
    void res_handler(Msg &msg) override;
};

#endif // !CLIENT_CMD_HANDLER_H

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

namespace oeaware {
class CmdHandler {
public:
    virtual void Handler(Msg &msg) = 0;
    virtual void ResHandler(Msg &msg) = 0;
};

class LoadHandler : public CmdHandler {
public:
    void Handler(Msg &msg) override;
    void ResHandler(Msg &msg) override;
private:
    void Check(const std::string &type);
    static std::unordered_set<std::string> types;
};

class QueryHandler : public CmdHandler {
public:
    void Handler(Msg &msg) override;
    void ResHandler(Msg &msg) override;
private:
    void PrintFormat();
};

class RemoveHandler : public CmdHandler {
public:
    void Handler(Msg &msg) override;
    void ResHandler(Msg &msg) override;
};


class QueryTopHandler : public CmdHandler {
public:
    void Handler(Msg &msg) override;
    void ResHandler(Msg &msg) override;
};

class EnabledHandler : public CmdHandler {
public:
    void Handler(Msg &msg) override;
    void ResHandler(Msg &msg) override;
};

class DisabledHandler : public CmdHandler {
public:
    void Handler(Msg &msg) override;
    void ResHandler(Msg &msg) override;
};

class ListHandler : public CmdHandler {
public:
    void Handler(Msg &msg) override;
    void ResHandler(Msg &msg) override;
};

class InstallHandler : public CmdHandler {
public:
    void Handler(Msg &msg) override;
    void ResHandler(Msg &msg) override;
};
}

#endif // !CLIENT_CMD_Handler_H

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
#include "client.h"

namespace oeaware {
void Client::CmdGroupsInit()
{
    cmdHandlerGroups.insert(std::make_pair('l', std::make_shared<LoadHandler>()));
    cmdHandlerGroups.insert(std::make_pair('q', std::make_shared<QueryHandler>()));
    cmdHandlerGroups.insert(std::make_pair('r', std::make_shared<RemoveHandler>()));
    cmdHandlerGroups.insert(std::make_pair('Q', std::make_shared<QueryTopHandler>()));
    cmdHandlerGroups.insert(std::make_pair('e', std::make_shared<EnabledHandler>()));
    cmdHandlerGroups.insert(std::make_pair('d', std::make_shared<DisabledHandler>()));
    cmdHandlerGroups.insert(std::make_pair('L', std::make_shared<ListHandler>()));
    cmdHandlerGroups.insert(std::make_pair('i', std::make_shared<InstallHandler>()));
}

bool Client::Init(int argc, char *argv[])
{
    if ((cmd = ArgParse::GetInstance().Init(argc, argv)) < 0) {
        return false;
    }
    CmdGroupsInit();
    return this->tcpSocket.Init();
}

void Client::RunCmd()
{
    Message msg;
    Message res;
    MessageHeader header;
    this->cmdHandler = cmdHandlerGroups[cmd];
    this->cmdHandler->Handler(msg);
    if (!this->tcpSocket.SendMsg(msg, header)) {
        return;
    }
    if (!this->tcpSocket.RecvMsg(res, header)) {
        return;
    }
    this->cmdHandler->ResHandler(res);
}
}

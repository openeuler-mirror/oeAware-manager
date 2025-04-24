/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef _ITA_UEID_H_
#define _ITA_UEID_H_
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <iostream>
#include "smc_common.h"
#include "smc_netlink.h"

class SmcOperator {
public:
    static SmcOperator *getInstance()
    {
        static SmcOperator instance;
        return &instance;
    }
    int InvokeUeid(int act);
    void SetEnable(int isEnable);
    int RunSmcAcc();
    int CheckSmcKo();
    int SmcInit();
    int SmcExit();
    int EnableSmcAcc();
    int DisableSmcAcc();
    int AbleSmcAcc(int isEnable);
    int InputPortList(const std::string &blackPortStr, const std::string &whitePortStr);
    bool IsSamePortList(const std::string &blackPortStr, const std::string &whitePortStr);
    void SetShortConnection(int value);
    int ReRunSmcAcc();

private:
    SmcOperator()
    {
        sprintf(targetEid, "%-32s", SMC_UEID_NAME);
    }
    ~SmcOperator()
    {
    }
    int act;
    int enable;
    struct nl_sock *sk;
    bool isInit;
    char targetEid[SMC_MAX_EID_LEN + 1];
    std::string blackPortList;
    std::string whitePortList;
    int shortConnection;
};

#endif /* UEID_H_ */

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
#include "smc_common.h"
#include "smc_netlink.h"

class SmcOperator {
public:
    static SmcOperator *getInstance()
    {
        static SmcOperator instance;
        return &instance;
    }
    int invoke_ueid(int act);
    void set_enable(int is_enable);
    int run_smc_acc(char *ko_path = NULL);
    int check_smc_ko();
    int smc_init();
    int smc_exit();
    int enable_smc_acc();
    int disable_smc_acc();
    int check_smc_acc_run();
    int able_smc_acc(int is_enable);
private:
    SmcOperator()
    {
        sprintf(target_eid, "%-32s", SMC_UEID_NAME);
    }
    ~SmcOperator()
    {
    }
    int act;
    int enable;
    struct nl_sock *sk;
    bool is_init;
    char target_eid[SMC_MAX_EID_LEN + 1];
};

#endif /* UEID_H_ */

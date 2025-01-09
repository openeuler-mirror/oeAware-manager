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
#ifndef ITA_SMC_LIBNETLINK_H_
#define ITA_SMC_LIBNETLINK_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>
#include <netlink/socket.h>
#include <netlink/msg.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>

class SmcNetlink {
public:
    static SmcNetlink *getInstance()
    {
        static SmcNetlink instance;
        return &instance;
    }
    int GenNlOpen();
    void GenNlClose();
    int GenNlEuidHandle(int cmd, char *ueid, int (*cb_handler)(struct nl_msg *msg, void *arg));
    bool CheckSkInvaild()
    {
        return sk != nullptr;
    }

private:
    SmcNetlink() : smc_id(0)
    {
    }
    ~SmcNetlink()
    {
    }
    int smc_id;
    struct nl_sock *sk;
};

#endif /* ITA_SMC_LIBNETLINK_H_ */

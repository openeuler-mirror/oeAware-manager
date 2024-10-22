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
#ifndef SMCTOOLS_COMMON_H
#define SMCTOOLS_COMMON_H

#define PF_SMC 43

#include <net/if.h>

#define SMC_MAX_EID_LEN 32      /* Max length of eid */

#define UEID_ADD 1 << 1
#define UEID_DEL 1 << 2
#define UEID_FLUSH 1 << 3
#define UEID_SHOW 1 << 4

/* Use for accessing non-socket information like */
/* SMC links, linkgroups and devices */
#define SMC_GENL_FAMILY_NAME "SMC_GEN_NETLINK"
#define SMC_GENL_FAMILY_VERSION 1

#define SMC_ENABLE 0
#define SMC_DISABLE 1

/* SMC_GENL_FAMILY commands */
enum {
    SMC_ACC_NL_GET_SYS_INFO = 1,
    SMC_ACC_NL_GET_LGR_SMCR,
    SMC_ACC_NL_GET_LINK_SMCR,
    SMC_ACC_NL_GET_LGR_SMCD,
    SMC_ACC_NL_GET_DEV_SMCD,
    SMC_ACC_NL_GET_DEV_SMCR,
    SMC_ACC_NL_GET_STATS,
    SMC_ACC_NL_GET_FBACK_STATS,
    SMC_ACC_NL_DUMP_UEID,
    SMC_ACC_NL_ADD_UEID,
    SMC_ACC_NL_REMOVE_UEID,
    SMC_ACC_NL_FLUSH_UEID,
    SMC_ACC_NL_DUMP_SEID,
    SMC_ACC_NL_ENABLE_SEID,
    SMC_ACC_NL_DISABLE_SEID,
};

/* SMC_ACC_NL_UEID attributes */
enum {
    SMC_ACC_NLA_EID_TABLE_UNSPEC,
    SMC_ACC_NLA_EID_TABLE_ENTRY, /* string */
    __SMC_ACC_NLA_EID_TABLE_MAX,
    SMC_ACC_NLA_EID_TABLE_MAX = __SMC_ACC_NLA_EID_TABLE_MAX - 1
};

extern int log_level;

#define pr_level(level, target, fmt, args...) \
    do {                                      \
        if (level <= log_level)               \
            fprintf(target, fmt, ##args);     \
    } while (0)

#define log_info(fmt, args...) pr_level(0, stdout, fmt, ##args)
#define log_warn(fmt, args...) pr_level(0, stderr, "\033[0;34mWARN: " fmt "\033[0m", ##args)
#define log_err(fmt, args...) pr_level(0, stderr, "\033[0;31mERROR: " fmt "\033[0m", ##args)
#define log_debug(fmt, args...) pr_level(2, stdout, "DEBUG: " fmt, ##args)

#define SMC_ACC_KO_PATH "/usr/lib/smc/smc_acc.ko"

#define SMC_UEID_NAME "SMCV2-OPENEULER-UEID"

#endif /* SMCTOOLS_COMMON_H */

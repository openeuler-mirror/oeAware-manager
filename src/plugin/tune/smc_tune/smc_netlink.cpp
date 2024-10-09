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

#include "smc_common.h"
#include "smc_netlink.h"

/* Operations on generic netlink sockets */

int SmcNetlink::gen_nl_open()
{
    int rc = EXIT_FAILURE;

    /* Allocate a netlink socket and connect to it */
    sk = nl_socket_alloc();
    if (!sk) {
        nl_perror(NLE_NOMEM, "Error");
        return rc;
    }
    rc = genl_connect(sk);
    log_debug("genl_connect rc value:%d \n", rc);
    if (rc) {
        nl_perror(rc, "Error");
        rc = EXIT_FAILURE;
        goto err1;
    }

    smc_id = genl_ctrl_resolve(sk, SMC_GENL_FAMILY_NAME);
    log_debug("get smc module smc_id value: %d \n", smc_id);
    if (smc_id < 0) {
        rc = EXIT_FAILURE;
        if (smc_id == -NLE_OBJ_NOTFOUND)
            log_err(" SMC module not loaded\n");
        else
            nl_perror(smc_id, "Error");
        goto err2;
    }

    return EXIT_SUCCESS;
err2:
    nl_close(sk);
err1:
    nl_socket_free(sk);
    return rc;
}

void SmcNetlink::gen_nl_close()
{
    if (sk) {
        nl_close(sk);
        nl_socket_free(sk);
        sk = NULL;
    }
}


static void handle_euid_err(int rc, int cmd) {
    if (rc == -NLE_OPNOTSUPP) {
        log_err(" operation not supported by kernel\n");
    } else if (cmd == SMC_ACC_NL_REMOVE_UEID) {
        switch(rc){
            case -NLE_OBJ_NOTFOUND: {
                log_warn("Warn: specified User EID is not defined\n");
                break;
            }
            case -NLE_AGAIN: {
                log_err(" the System EID was activated because the last User EID was removed\n");
                break;
            }
            default: {
                log_warn("Warn: specified User EID is not defined\n");
                break;
            }
        }
    } else if (cmd == SMC_ACC_NL_ADD_UEID) {
        switch(rc){
            case -NLE_INVAL: {
                log_err(" specified User EID was rejected by the kernel\n");
                break;
            }
            case -NLE_NOMEM: {
                log_err(" specified User EID was rejected by the kernel\n");
                break;
            }
            case -NLE_RANGE: {
                log_err(" specified User EID was rejected because the maximum number of User EIDs is reached\n");
                break;
            }
            case -NLE_EXIST: {
                log_warn(" specified User EID is already defined\n");
                break;
            }
            default: {
                nl_perror(rc, "Error");
                break;
            }
        }
    } else {
        nl_perror(rc, "Error");
    }
}


int SmcNetlink::gen_nl_euid_handle(int cmd, char *ueid, int (*cb_handler)(nl_msg *msg, void *arg))
{
    int rc = EXIT_FAILURE, nlmsg_flags = 0;
    struct nl_msg *msg;

    if (!check_sk_invaild()) {
        rc = EXIT_FAILURE;
        log_err("The SmcNetlink sk is not init yet\n");
        return rc;
    }

    nl_socket_modify_cb(sk, NL_CB_VALID, NL_CB_CUSTOM, cb_handler, NULL);

    msg = nlmsg_alloc();
    if (!msg) {
        nl_perror(NLE_NOMEM, "Error");
        rc = EXIT_FAILURE;
        goto err;
    }

    if (cmd == SMC_ACC_NL_DUMP_UEID)
        nlmsg_flags = NLM_F_DUMP;

    if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, smc_id, 0, nlmsg_flags, cmd, SMC_GENL_FAMILY_VERSION)) {
        nl_perror(rc, "Error");
        rc = EXIT_FAILURE;
        goto err;
    }

    if (ueid && ueid[0]) {
        rc = nla_put_string(msg, SMC_ACC_NLA_EID_TABLE_ENTRY, ueid);
        log_debug("nla put string rc value: %d \n", rc);
        if (rc < 0) {
            nl_perror(rc, "Error");
            rc = EXIT_FAILURE;
            goto err;
        }
    }

    log_debug("send message\n");
    rc = nl_send_auto(sk, msg);
    if (rc < 0) {
        nl_perror(rc, "Error");
        rc = EXIT_FAILURE;
        goto err;
    }

    rc = nl_recvmsgs_default(sk);
    log_debug("receive reply message, get rc value : %d\n", rc);
    if (rc < 0) {
        handle_euid_err(rc, cmd);
        rc = EXIT_FAILURE;
        goto err;
    }

    nlmsg_free(msg);
    return EXIT_SUCCESS;
err:
    nlmsg_free(msg);
    return rc;
}

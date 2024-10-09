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

#include "smc_ueid.h"

const struct nla_policy smc_gen_ueid_policy[SMC_ACC_NLA_EID_TABLE_MAX + 1] = {
    [SMC_ACC_NLA_EID_TABLE_UNSPEC] = {.type = NLA_UNSPEC},
    [SMC_ACC_NLA_EID_TABLE_ENTRY]  = {.type = NLA_NUL_STRING},
};

static int handle_gen_ueid_reply(struct nl_msg *msg, void *arg)
{
    struct nlattr *attrs[SMC_ACC_NLA_EID_TABLE_ENTRY + 1];
    struct nlmsghdr *hdr = nlmsg_hdr(msg);
    int rc               = NL_OK;
    (void *)arg;
    if (genlmsg_parse(hdr, 0, attrs, SMC_ACC_NLA_EID_TABLE_ENTRY, (struct nla_policy *)smc_gen_ueid_policy) < 0) {
        log_err(" invalid data returned: smc_gen_ueid_policy\n");
        nl_msg_dump(msg, stderr);
        return NL_STOP;
    }

    if (!attrs[SMC_ACC_NLA_EID_TABLE_ENTRY])
        return NL_STOP;

    printf("%s\n", nla_get_string(attrs[SMC_ACC_NLA_EID_TABLE_ENTRY]));
    return rc;
}


static inline int file_exist(const char *path)
{
    return access(path, F_OK) == 0;
}

static int exec(const char *cmd, size_t len)
{
    FILE *output;
    char buffer[128];
    if (len > sizeof(buffer)) {
        log_err("Too many words\n");
    }
    int status;

    output = popen(cmd, "r");
    if (output == NULL) {
        return -1;
    }
    while (fgets(buffer, sizeof(buffer), output) != NULL) {
    }

    status = pclose(output);
    log_info(" command: %s, status:%d\n", cmd, WEXITSTATUS(status));
    return WEXITSTATUS(status);
}

static inline int simple_exec(char *cmd, ...)
{
    char cmd_format[512];
    va_list valist;

    va_start(valist, cmd);
    if (!vsprintf(cmd_format, cmd, valist)) {
        log_err("get cmd format error.\n");
        return -1;
    }
    va_end(valist);
    return exec(cmd_format, strlen(cmd_format));
}

int SmcOperator::invoke_ueid(int act)
{
    int rc = EXIT_SUCCESS;
    if (strlen(target_eid) == 0) {
        log_err("target_eid is not set yet\n");
        rc = EXIT_FAILURE;
        return rc;
    }

    switch (act) {
        case UEID_ADD: {
            rc = SmcNetlink::getInstance()->gen_nl_euid_handle(SMC_ACC_NL_ADD_UEID, target_eid, handle_gen_ueid_reply);
            break;
        }
        case UEID_DEL: {
            rc = SmcNetlink::getInstance()->gen_nl_euid_handle(SMC_ACC_NL_REMOVE_UEID, target_eid,
                                                               handle_gen_ueid_reply);
            break;
        }
        case UEID_FLUSH: {
            rc = SmcNetlink::getInstance()->gen_nl_euid_handle(SMC_ACC_NL_FLUSH_UEID, NULL, handle_gen_ueid_reply);
            break;
        }
        case UEID_SHOW: {
            rc = SmcNetlink::getInstance()->gen_nl_euid_handle(SMC_ACC_NL_DUMP_UEID, NULL, handle_gen_ueid_reply);
            break;
        }
        default: {
            log_err(" Unknown command\n");
            break;
        }
    }

    return rc;
}

void SmcOperator::set_enable(int is_enable)
{
    enable = is_enable;
}

int SmcOperator::run_smc_acc(char *ko_path)
{
    int rc = EXIT_SUCCESS;

    if (enable == SMC_DISABLE) {
        if (!simple_exec((char *)"lsmod | grep -w smc_acc")) {
            rc = simple_exec((char *)"rmmod smc_acc");
            goto out;
        }
        goto out;
    }
    if (ko_path) {
        if (!file_exist(ko_path)) {
            log_err(" %s is not exist.\n", ko_path);
            rc = EXIT_FAILURE;
            goto out;
        }
        if (simple_exec((char *)"lsmod | grep -w smc_acc")) {
            rc = simple_exec((char *)"insmod %s", ko_path);
            goto out;
        }

    } else {
        if (!file_exist(SMC_ACC_KO_PATH)) {
            log_err(" %s is not exist.\n", SMC_ACC_KO_PATH);
            rc = EXIT_FAILURE;
            goto out;
        }
        if (simple_exec((char *)"lsmod | grep -w smc_acc")) {
            rc = simple_exec((char *)"insmod  %s", SMC_ACC_KO_PATH);
        }
    }

out:
    return rc;
}

int SmcOperator::check_smc_ko()
{
    int rc = EXIT_SUCCESS;
    if (simple_exec((char *)"lsmod | grep -w smc") == EXIT_FAILURE) {
        rc = EXIT_FAILURE;
        log_err("smc ko is not running\n");
    }
    if (simple_exec((char *)"lsmod | grep -w smc_acc") == EXIT_FAILURE) {
        rc = EXIT_FAILURE;
        log_err("smc_acc ko is not running\n");
    }

    return rc;
}

int SmcOperator::smc_init()
{
    int rc = EXIT_SUCCESS;
    if (is_init)
        goto out;
    if (SmcNetlink::getInstance()->gen_nl_open() == EXIT_SUCCESS) {
        is_init = true;
    } else {
        rc = EXIT_FAILURE;
    }
out:
    return rc;
}


int SmcOperator::smc_exit()
{
    SmcNetlink::getInstance()->gen_nl_close();
    is_init = false;
    return 0;
}

int SmcOperator::able_smc_acc(int is_enable) 
{
    int rc = EXIT_FAILURE;
    if (smc_init() == EXIT_FAILURE) {
        log_err("failed to exec init\n");
        return rc;
    }

    set_enable(is_enable);
    if (invoke_ueid(UEID_DEL) == EXIT_SUCCESS) {
        log_info(" success to del smc ueid %s\n", SMC_UEID_NAME);
    }

    if (enable == SMC_ENABLE) {
        if (invoke_ueid(UEID_ADD) == EXIT_SUCCESS) {
            log_info("INFO: success to add smc ueid %s\n", SMC_UEID_NAME);
        } else {
            log_err(" failed to add smc\n");
            goto out;
        }
    }

    if (run_smc_acc() == EXIT_FAILURE) {
        log_err("failed to run smc acc\n");
        goto out;
    }
    rc = EXIT_SUCCESS;
out:
    smc_exit();
    return rc;
}

int SmcOperator::enable_smc_acc()
{
    return able_smc_acc(SMC_ENABLE);
}

int SmcOperator::disable_smc_acc()
{
    return able_smc_acc(SMC_DISABLE);
}

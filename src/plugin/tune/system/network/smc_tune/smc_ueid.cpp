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

const std::vector<std::string> whiteList ={
    "lsmod",
    "grep",
    "rmmod",
    "insmod"
};

static int HandleGenUeidReply(struct nl_msg *msg, void *arg)
{
    struct nlattr *attrs[SMC_ACC_NLA_EID_TABLE_ENTRY + 1];
    struct nlmsghdr *hdr = nlmsg_hdr(msg);
    int rc               = NL_OK;
    (void)arg;
    if (genlmsg_parse(hdr, 0, attrs, SMC_ACC_NLA_EID_TABLE_ENTRY, (struct nla_policy *)smc_gen_ueid_policy) < 0) {
        SMCLOG_ERROR(" invalid data returned: smc_gen_ueid_policy");
        nl_msg_dump(msg, stderr);
        return NL_STOP;
    }

    if (!attrs[SMC_ACC_NLA_EID_TABLE_ENTRY])
        return NL_STOP;

    SMCLOG_INFO("ueid : " << nla_get_string(attrs[SMC_ACC_NLA_EID_TABLE_ENTRY]));
    return rc;
}

static inline int FileExist(const char *path)
{
    return access(path, F_OK) == 0;
}

static inline bool IsValidCmd(const std::string& cmd)
{
    for (const auto& allowed_cmd : whiteList) {
        if(cmd.find(allowed_cmd) != std::string::npos) {
            return true;
        }
    }
    return false;
}

static int Exec(const char *cmd, size_t len)
{
    FILE *output;
    char buffer[128];
    if (len > sizeof(buffer)) {
        SMCLOG_ERROR("Too many words");
    }
    int status;

    if (!IsValidCmd(cmd)) {
        SMCLOG_ERROR("Invalid command: " << cmd);
        return -1;
    }
    output = popen(cmd, "r");
    if (output == NULL) {
        return -1;
    }
    while (fgets(buffer, sizeof(buffer), output) != NULL) {
    }

    status = pclose(output);
    SMCLOG_INFO(" command: " << cmd << ", status:" << WEXITSTATUS(status));
    return WEXITSTATUS(status);
}

static inline int SimpleExec(char *cmd, ...)
{
    char cmd_format[512];
    va_list valist;

    va_start(valist, cmd);
    if (!vsprintf(cmd_format, cmd, valist)) {
        SMCLOG_ERROR("get cmd format error.");
        return -1;
    }
    va_end(valist);
    return Exec(cmd_format, strlen(cmd_format));
}

int SmcOperator::InvokeUeid(int act)
{
    int rc = EXIT_SUCCESS;
    if (strlen(targetEid) == 0) {
        SMCLOG_ERROR("targetEid is not set yet");
        rc = EXIT_FAILURE;
        return rc;
    }

    switch (act) {
        case UEID_ADD: {
            rc = SmcNetlink::getInstance()->GenNlEuidHandle(SMC_ACC_NL_ADD_UEID, targetEid, HandleGenUeidReply);
            break;
        }
        case UEID_DEL: {
            rc = SmcNetlink::getInstance()->GenNlEuidHandle(SMC_ACC_NL_REMOVE_UEID, targetEid,
                                                               HandleGenUeidReply);
            break;
        }
        case UEID_FLUSH: {
            rc = SmcNetlink::getInstance()->GenNlEuidHandle(SMC_ACC_NL_FLUSH_UEID, NULL, HandleGenUeidReply);
            break;
        }
        case UEID_SHOW: {
            rc = SmcNetlink::getInstance()->GenNlEuidHandle(SMC_ACC_NL_DUMP_UEID, NULL, HandleGenUeidReply);
            break;
        }
        default: {
            SMCLOG_ERROR(" Unknown command");
            break;
        }
    }

    return rc;
}

void SmcOperator::SetEnable(int isEnable)
{
    enable = isEnable;
}

int SmcOperator::RunSmcAcc()
{
    int rc = EXIT_SUCCESS;
    std::stringstream args;

    if (enable == SMC_DISABLE) {
        if (!SimpleExec((char *)"lsmod | grep -w smc_acc")) {
            rc = SimpleExec((char *)"rmmod smc_acc");
            goto out;
        }
        goto out;
    }

    if (!blackPortList.empty()) {
        args << "black_port_list_param=\"" << blackPortList << "\" ";
    }
    if (!whitePortList.empty()) {
        args << "white_port_list_param=\"" << whitePortList << "\" ";
    }
    args << "short_connection=" << shortConnection;
    if (!FileExist(SMC_ACC_KO_PATH)) {
        SMCLOG_ERROR(SMC_ACC_KO_PATH << "  is not exist.");
        rc = EXIT_FAILURE;
        goto out;
    }
    if (SimpleExec((char *)"lsmod | grep -w smc_acc")) {
        rc = SimpleExec((char *)"insmod  %s %s", SMC_ACC_KO_PATH, args.str().c_str());
    }

out:
    return rc;
}

int SmcOperator::CheckSmcKo()
{
    int rc = EXIT_SUCCESS;
    if (SimpleExec((char *)"lsmod | grep -w smc") == EXIT_FAILURE) {
        rc = EXIT_FAILURE;
        SMCLOG_ERROR("smc ko is not running");
    }
    if (SimpleExec((char *)"lsmod | grep -w smc_acc") == EXIT_FAILURE) {
        rc = EXIT_FAILURE;
        SMCLOG_ERROR("smc_acc ko is not running");
    }

    return rc;
}

int SmcOperator::SmcInit()
{
    int rc = EXIT_SUCCESS;
    if (isInit)
        goto out;
    if (SmcNetlink::getInstance()->GenNlOpen() == EXIT_SUCCESS) {
        isInit = true;
    } else {
        rc = EXIT_FAILURE;
    }
out:
    return rc;
}

int SmcOperator::SmcExit()
{
    SmcNetlink::getInstance()->GenNlClose();
    isInit = false;
    return 0;
}

int SmcOperator::AbleSmcAcc(int isEnable)
{
    int rc = EXIT_FAILURE;
    if (SmcInit() == EXIT_FAILURE) {
        SMCLOG_ERROR("failed to Exec init");
        return rc;
    }

    SetEnable(isEnable);
    if (InvokeUeid(UEID_DEL) == EXIT_SUCCESS) {
        SMCLOG_INFO(" success to del smc ueid " << SMC_UEID_NAME);
    }

    if (enable == SMC_ENABLE) {
        if (InvokeUeid(UEID_ADD) == EXIT_SUCCESS) {
            SMCLOG_INFO("success to add smc ueid " << SMC_UEID_NAME);
        } else {
            SMCLOG_ERROR(" failed to add smc");
            goto out;
        }
    }

    if (RunSmcAcc() == EXIT_FAILURE) {
        SMCLOG_ERROR("failed to run smc acc");
        goto out;
    }
    rc = EXIT_SUCCESS;
out:
    SmcExit();
    return rc;
}

int SmcOperator::EnableSmcAcc()
{
    return AbleSmcAcc(SMC_ENABLE);
}

int SmcOperator::DisableSmcAcc()
{
    return AbleSmcAcc(SMC_DISABLE);
}

int SmcOperator::InputPortList(const std::string &blackPortStr, const std::string &whitePortStr)
{
    blackPortList = blackPortStr;
    whitePortList = whitePortStr;
    return 0;
}

bool SmcOperator::IsSamePortList(const std::string &blackPortStr, const std::string &whitePortStr)
{
    return (blackPortList == blackPortStr) && (whitePortList == whitePortStr);
}

void SmcOperator::SetShortConnection(int value)
{
    this->shortConnection = value;
}

int SmcOperator::ReRunSmcAcc()
{
    int rc = EXIT_SUCCESS;
    std::stringstream args;

    if (!SimpleExec((char *)"lsmod | grep -w smc_acc")) {
        rc = SimpleExec((char *)"rmmod smc_acc");
    }

    if (!blackPortList.empty()) {
        args << "black_port_list_param=\"" << blackPortList << "\" ";
    }
    if (!whitePortList.empty()) {
        args << "white_port_list_param=\"" << whitePortList << "\" ";
    }
    args << "short_connection=" << shortConnection;
    SMCLOG_INFO("ReRunSmcAcc args:" << args.str());
    if (!FileExist(SMC_ACC_KO_PATH)) {
        SMCLOG_ERROR(SMC_ACC_KO_PATH << " is not exist.");
        rc = EXIT_FAILURE;
        goto out;
    }
    if (SimpleExec((char *)"lsmod | grep -w smc_acc")) {
        rc = SimpleExec((char *)"insmod  %s %s", SMC_ACC_KO_PATH, args.str().c_str());
    }

out:
    return rc;
}

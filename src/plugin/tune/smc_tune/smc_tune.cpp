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
#include <stdlib.h>
#include "smc_ueid.h"
#include "smc_netlink.h"
#include "smc_common.h"
#include "interface.h"

int log_level = 0;


#define SMC_OP SmcOperator::getInstance()

class SmcInterface {
public:
    static SmcInterface *getInstance()
    {
        static SmcInterface instance;
        return &instance;
    }
    static const struct DataRingBuf *get_ring_buf()
    {
        return nullptr;
    }
    static bool enable()
    {
        return SMC_OP->enable_smc_acc() == EXIT_SUCCESS ? true : false;
    }
    static void disable()
    {
        if (SMC_OP->disable_smc_acc() == EXIT_FAILURE) {
            log_err("failed to disable smc acc\n");
        } else {
            log_info("success to disable smc_acc \n");
        }
    }
    static const char *get_name()
    {
        return (char *)"smc_tune";
    }
    static const char *get_description()
    {
        return nullptr;
    }
    static void run(const Param *param)
    {
        (void *)param;
    }
    static int get_priority()
    {
        return 2;
    }
    static int get_period()
    {
        return 5000;
    }
    static const char *get_dep()
    {
        return nullptr;
    }
    static const char *get_version()
    {
        return nullptr;
    }

private:
    SmcInterface()
    {
    }
    ~SmcInterface()
    {
    }
};

static struct Interface smc_tune_interface = {
    .get_version     = SmcInterface::get_version,
    .get_name        = SmcInterface::get_name,
    .get_description = SmcInterface::get_description,
    .get_dep         = SmcInterface::get_dep,
    .get_priority    = SmcInterface::get_priority,
    .get_type        = nullptr,
    .get_period      = SmcInterface::get_period,
    .enable          = SmcInterface::enable,
    .disable         = SmcInterface::disable,
    .get_ring_buf    = SmcInterface::get_ring_buf,
    .run             = SmcInterface::run,
};

extern "C" int get_instance(Interface **ins)
{
    *ins = &smc_tune_interface;
    return 1;
}

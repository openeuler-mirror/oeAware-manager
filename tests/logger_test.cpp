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
#include <gtest/gtest.h>
#include "logger.h"

TEST(Serialize, Logger)
{
    oeaware::Logger::GetInstance().Init("./", 1);
    oeaware::Logger::GetInstance().Register("test1");
    auto logger = oeaware::Logger::GetInstance().Get("test1");
    DEBUG(logger, "Debug world");
    INFO(logger, "Hello world");
    WARN(logger, "wrang world");
    ERROR(logger, "Error world");
}

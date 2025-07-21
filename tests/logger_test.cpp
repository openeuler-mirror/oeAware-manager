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
#include "oeaware/interface.h"
bool LoggerTestLevelOutHelp(int level)
{
    auto logger = oeaware::Logger::GetInstance().Get("LoggerTestLevelOut");
    if (!oeaware::Logger::GetInstance().ChangeAllLogLevel(level)) {
        std::cout << "Change log level to " << level << " failed" << std::endl;
        return false;
    }
    
    TRACE(logger, "Trace log output at level " << level);
    DEBUG(logger, "Debug log output at level " << level);
    INFO(logger, "Info log output at level " << level);
    WARN(logger, "Warn log output at level " << level);
    ERROR(logger, "Error log output at level " << level);
    FATAL(logger, "Fatal log output at level " << level);
    return true;
}

TEST(LoggerTestLevelOut, Logger)
{
    oeaware::Logger::GetInstance().Init("./", oeaware::OE_TRACE_LEVEL);
    oeaware::Logger::GetInstance().Register("LoggerTestLevelOut");
    bool ret = true;
    std::cout << "LoggerTestLevelOut oeaware::OE_TRACE_LEVEL" << std::endl;
    ret = LoggerTestLevelOutHelp(oeaware::OE_TRACE_LEVEL);
    EXPECT_EQ(ret, true);
    std::cout << "LoggerTestLevelOut oeaware::OE_DEBUG_LEVEL" << std::endl;
    ret = LoggerTestLevelOutHelp(oeaware::OE_DEBUG_LEVEL);
    EXPECT_EQ(ret, true);
    std::cout << "LoggerTestLevelOut oeaware::OE_INFO_LEVEL" << std::endl;
    ret = LoggerTestLevelOutHelp(oeaware::OE_INFO_LEVEL);
    EXPECT_EQ(ret, true);
    std::cout << "LoggerTestLevelOut oeaware::OE_WARN_LEVEL" << std::endl;
    ret = LoggerTestLevelOutHelp(oeaware::OE_WARN_LEVEL);
    EXPECT_EQ(ret, true);
    std::cout << "LoggerTestLevelOut oeaware::OE_ERROR_LEVEL" << std::endl;
    ret = LoggerTestLevelOutHelp(oeaware::OE_ERROR_LEVEL);
    EXPECT_EQ(ret, true);
    std::cout << "LoggerTestLevelOut oeaware::OE_FATAL_LEVEL" << std::endl;
    ret = LoggerTestLevelOutHelp(oeaware::OE_FATAL_LEVEL);
    EXPECT_EQ(ret, true);
    std::cout << "LoggerTestLevelOut oeaware::OE_MAX_LOG_LEVELS" << std::endl;
    ret = LoggerTestLevelOutHelp(oeaware::OE_MAX_LOG_LEVELS);
    EXPECT_EQ(ret, false);
}

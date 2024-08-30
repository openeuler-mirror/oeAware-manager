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
#include "logger.h"
#include <sys/stat.h>

namespace oeaware {
Logger::Logger() noexcept
{
    logger = log4cplus::Logger::getInstance("oeAware");
    log4cplus::SharedAppenderPtr appender(new log4cplus::ConsoleAppender());
    appender->setName("console");
    log4cplus::tstring pattern = LOG4CPLUS_TEXT("%D{%m/%d/%y %H:%M:%S} [%t] %-5p %c - %m"
#ifdef OEAWARE_DEBUG 
    " [%l]"
#endif
     "%n");
    std::unique_ptr<log4cplus::Layout> layout = std::make_unique<log4cplus::PatternLayout>(pattern);
    appender->setLayout(std::move(layout));
    logger.setLogLevel(log4cplus::INFO_LOG_LEVEL);
    logger.addAppender(appender);
}

void Logger::Init(const std::string &path, const int level)
{
    log4cplus::SharedAppenderPtr appender(new log4cplus::FileAppender(path + "/server.log", std::ios_base::app));
    appender->setName("file");
    log4cplus::tstring pattern = LOG4CPLUS_TEXT("%D{%m/%d/%y %H:%M:%S} [%t] %-5p %c - %m"
#ifdef OEAWARE_DEBUG
    " [%l]"
#endif
    "%n");
    std::unique_ptr<log4cplus::Layout> layout = std::make_unique<log4cplus::PatternLayout>(pattern);
    appender->setLayout(std::move(layout));
    logger.setLogLevel(level);
    logger.addAppender(appender);
}
}

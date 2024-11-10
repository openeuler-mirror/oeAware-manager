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
log4cplus::SharedAppenderPtr Logger::GenerateAppender(log4cplus::Appender *appender)
{
     log4cplus::SharedAppenderPtr sharedAppenderPtr(appender);
     log4cplus::tstring pattern = LOG4CPLUS_TEXT("%D{%m/%d/%y %H:%M:%S} [%T] %-5p %c - %m"
#ifdef OEAWARE_DEBUG
    " [%l]"
#endif
     "%n");
     std::unique_ptr<log4cplus::Layout> layout = std::make_unique<log4cplus::PatternLayout>(pattern);
     sharedAppenderPtr->setLayout(std::move(layout));
     return sharedAppenderPtr;
}

void Logger::Register(const std::string &name)
{
    auto logger = log4cplus::Logger::getInstance(name);
    logger.setLogLevel(logLevel);
    logger.addAppender(GenerateAppender(new log4cplus::ConsoleAppender()));
    logger.addAppender(GenerateAppender(new log4cplus::FileAppender(logPath + "/server.log", std::ios_base::app)));
    loggers.insert({name, logger});
}

int Logger::Init(const std::string &path, const int level)
{
    int len = sizeof(LOG_LEVELS) / sizeof(LOG_LEVELS[0]);
    if (level < 0 || level >=  len) {
        return -1;
    }
    logLevel = LOG_LEVELS[level];
    logPath = path;
    return 0;
}

std::string LogText(const std::string &text)
{
    if (text.empty()) {
        return "nullptr";
    }
    return text;
}
}

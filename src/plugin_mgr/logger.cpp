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

Logger::Logger() {
    logger = log4cplus::Logger::getInstance("oeAware");
    log4cplus::SharedAppenderPtr appender(new log4cplus::ConsoleAppender()); 
    appender->setName("console");
    log4cplus::tstring pattern = LOG4CPLUS_TEXT("%D{%m/%d/%y %H:%M:%S} [%t] %-5p %c - %m"
#ifdef OEAWARE_DEBUG 
    " [%l]"
#endif
     "%n");
    appender->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(pattern)));
    logger.setLogLevel(log4cplus::INFO_LOG_LEVEL);
    logger.addAppender(appender);
}

void Logger::init(std::shared_ptr<Config> config) {   
    log4cplus::SharedAppenderPtr appender(new log4cplus::FileAppender(config->get_log_path() + "/server.log"));
    appender->setName("file");
    log4cplus::tstring pattern = LOG4CPLUS_TEXT("%D{%m/%d/%y %H:%M:%S} [%t] %-5p %c - %m"
#ifdef OEAWARE_DEBUG 
    " [%l]"
#endif
    "%n");
    appender->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(pattern)));
    logger.setLogLevel(config->get_log_level());
    logger.addAppender(appender);
}
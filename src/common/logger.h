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
#ifndef COMMON_LOGGER_H
#define COMMON_LOGGER_H
#include <memory>
#include <unordered_map>
#include <log4cplus/log4cplus.h>
#include <log4cplus/loglevel.h>
#include "default_path.h"

namespace oeaware {
#define INFO(logger, fmt) LOG4CPLUS_INFO(logger, fmt)
#define DEBUG(logger, fmt) LOG4CPLUS_DEBUG(logger, fmt)
#define WARN(logger, fmt) LOG4CPLUS_WARN(logger, fmt)
#define ERROR(logger, fmt) LOG4CPLUS_ERROR(logger, fmt)
#define FATAL(logger, fmt) LOG4CPLUS_FATAL(logger, fmt)
class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    static Logger& GetInstance()
    {
        static Logger logger;
        return logger;
    }
    int Init(const std::string &path, const int level);
    void Register(const std::string &name);
    log4cplus::Logger Get(const std::string &name)
    {
        return loggers[name];
    }
private:
    Logger() noexcept
    {
        logLevel = log4cplus::INFO_LOG_LEVEL;
        logPath = DEFAULT_LOG_PATH;
    };
    log4cplus::SharedAppenderPtr GenerateAppender(log4cplus::Appender *appender);
private:
    std::unordered_map<std::string, log4cplus::Logger> loggers;
    std::string logPath;
    int logLevel;
    const int LOG_LEVELS[6] = {log4cplus::TRACE_LOG_LEVEL, log4cplus::DEBUG_LOG_LEVEL, log4cplus::INFO_LOG_LEVEL,
        log4cplus::WARN_LOG_LEVEL, log4cplus::ERROR_LOG_LEVEL, log4cplus::FATAL_LOG_LEVEL};
};
std::string LogText(const std::string &text);
}

#endif

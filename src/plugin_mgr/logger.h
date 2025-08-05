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
#ifndef OEAWARE_LOGGER_H
#define OEAWARE_LOGGER_H
#include <memory>
#include <unordered_map>
#include <log4cplus/log4cplus.h>
#include <log4cplus/loglevel.h>
#include <oeaware/default_path.h>

namespace oeaware {

using OeLogLevel = enum {
    OE_TRACE_LEVEL = 0,
    OE_DEBUG_LEVEL,
    OE_INFO_LEVEL,
    OE_WARN_LEVEL ,
    OE_ERROR_LEVEL,
    OE_FATAL_LEVEL,
    OE_MAX_LOG_LEVELS
};

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
        if (loggers.count(name) == 0) {
            Register(name);
        }
        return loggers[name];
    }

    bool ChangeAllLogLevel(const int level)
    {
        int len = sizeof(LOG_LEVELS) / sizeof(LOG_LEVELS[0]);
        if (level < 0 || level >= len) {
            return false;
        }
        int log4Level = LOG_LEVELS[level];
        for (auto &it : loggers) {
            // Main 是特殊的日志，永远是INFO级别
            if (it.first == "Main") {
                continue;
            }
            it.second.setLogLevel(log4Level);
        }
        logLevel = log4Level;
        return true;
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
    const int LOG_LEVELS[OE_MAX_LOG_LEVELS] = {log4cplus::TRACE_LOG_LEVEL, log4cplus::DEBUG_LOG_LEVEL, log4cplus::INFO_LOG_LEVEL,
        log4cplus::WARN_LOG_LEVEL, log4cplus::ERROR_LOG_LEVEL, log4cplus::FATAL_LOG_LEVEL};
};
std::string LogText(const std::string &text);
void RedirectBpfLog();
}

#endif

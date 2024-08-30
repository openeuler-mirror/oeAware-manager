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
#ifndef PLUGIN_MGR_LOGGER_H
#define PLUGIN_MGR_LOGGER_H
#include <memory>
#include <log4cplus/log4cplus.h>

namespace oeaware {
#define INFO(fmt) LOG4CPLUS_INFO(g_logger.Get(), fmt)
#define DEBUG(fmt) LOG4CPLUS_DEBUG(g_logger.Get(), fmt)
#define WARN(fmt) LOG4CPLUS_WARN(g_logger.Get(), fmt)
#define ERROR(fmt) LOG4CPLUS_ERROR(g_logger.Get(), fmt)
#define FATAL(fmt) LOG4CPLUS_FATAL(g_logger.Get(), fmt)

class Logger {
public:
    Logger() noexcept;
    void Init(const std::string &path, const int level);
    log4cplus::Logger Get()
    {
        return logger;
    }
private:
    log4cplus::Logger logger;
    log4cplus::Initializer initalizer;
};
}
extern oeaware::Logger g_logger;

#endif
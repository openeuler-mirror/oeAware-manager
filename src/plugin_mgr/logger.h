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

#include "config.h"
#include <memory>
#include <log4cplus/log4cplus.h>

#define INFO(fmt) LOG4CPLUS_INFO(logger.get(), fmt)
#define DEBUG(fmt) LOG4CPLUS_DEBUG(logger.get(), fmt)
#define WARN(fmt) LOG4CPLUS_WARN(logger.get(), fmt)
#define ERROR(fmt) LOG4CPLUS_ERROR(logger.get(), fmt)
#define FATAL(fmt) LOG4CPLUS_FATAL(logger.get(), fmt)

class Logger {
public:
    Logger();    
    void init(std::shared_ptr<Config> config);
    log4cplus::Logger get() {
        return logger;
    }
private:
    log4cplus::Logger logger;
    log4cplus::Initializer initializer;
};

extern Logger logger;

#endif // !PLUGIN_MGR_LOGGER_H
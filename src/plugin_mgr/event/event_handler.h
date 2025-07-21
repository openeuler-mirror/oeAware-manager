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
#ifndef PLUGIN_MGR_EVENT_EVENT_HANDLER_H
#define PLUGIN_MGR_EVENT_EVENT_HANDLER_H
#include "event.h"
#include "memory_store.h"
#include "logger.h"
#include "error_code.h"

namespace oeaware {
class Handler {
public:
    virtual ~Handler() = default;
    virtual EventResult Handle(const Event &event) = 0;
    static std::shared_ptr<MemoryStore> memoryStore;
    static log4cplus::Logger logger;
};
}

#endif

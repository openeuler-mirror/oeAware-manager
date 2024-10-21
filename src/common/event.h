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
#ifndef PLUGIN_MGR_EVENT_EVENT_H
#define PLUGIN_MGR_EVENT_EVENT_H
#include <vector>
#include "message_protocol.h"
#include "safe_queue.h"

namespace oeaware {
enum class EventType {
    INTERNAL,
    EXTERNAL,
};

/* Event processed by the server. */
struct Event {
    Event() : type(EventType::EXTERNAL) {}
    explicit Event(Opt opt) : opt(opt) {}
    Event(Opt opt, EventType type) : opt(opt), type(type) {}
    Event(Opt opt, const std::vector<std::string> &payload) : opt(opt), payload(payload) {}
    Event(Opt opt, EventType type, const std::vector<std::string> &payload) : opt(opt), type(type), payload(payload) {}
    Opt opt;
    EventType type;
    std::vector<std::string> payload;
};

struct EventResult {
    EventResult() { }
    Opt opt;
    std::vector<std::string> payload;
};

using EventQueue = std::shared_ptr<SafeQueue<Event>>;
using EventResultQueue = std::shared_ptr<SafeQueue<EventResult>>;
}

#endif

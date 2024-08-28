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
#include "error_code.h"
#include "message_protocol.h"

namespace oeaware {
enum class EventType {
    INTERNAL,
    EXTERNAL,
};

/* Event processed by the server. */
class Event {
public:
    Event() : type(EventType::EXTERNAL) {}
    explicit Event(Opt opt) : opt(opt) {}
    Event(Opt opt, EventType type) : opt(opt), type(type) {}
    Event(Opt opt, const std::vector<std::string> &payload) : opt(opt), payload(payload) {}
    Event(Opt opt, EventType type, const std::vector<std::string> &payload) : opt(opt), type(type), payload(payload) {}
    Opt GetOpt() const
    {
        return this->opt;
    }
    void SetOpt(Opt newOpt)
    {
        this->opt = newOpt;
    }
    void SetType(EventType newType)
    {
        this->type = newType;
    }
    EventType GetType() const
    {
        return this->type;
    }
    void AddPayload(std::string s)
    {
        this->payload.emplace_back(s);
    }
    std::string GetPayload(int index) const
    {
        return this->payload[index];
    }
    int GetPayloadLen() const
    {
        return this->payload.size();
    }
private:
    Opt opt;
    EventType type;
    std::vector<std::string> payload;
};

class EventResult {
public:
    EventResult() { }
    Opt GetOpt()
    {
        return this->opt;
    }
    void SetOpt(Opt newOpt)
    {
        this->opt = newOpt;
    }
    void AddPayload(const std::string &context)
    {
        payload.emplace_back(context);
    }
    std::string GetPayload(int index) const
    {
        return this->payload[index];
    }
    int GetPayloadLen() const
    {
        return this->payload.size();
    }
private:
    Opt opt;
    std::vector<std::string> payload;
};
}

#endif

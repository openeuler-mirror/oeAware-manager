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
#ifndef COMMON_TOPIC_H
#define COMMON_TOPIC_H
#include "utils.h"
#include "serialize.h"

namespace oeaware {
struct Topic {
    std::string instanceName;
    std::string topicName;
    std::string params;
    void Serialize(oeaware::OutStream &out) const
    {
        out << instanceName << topicName << params;
    }
    void Deserialize(oeaware::InStream &in)
    {
        in >> instanceName >> topicName >> params;
    }
    std::string GetDataType() const
    {
        if (topicName.empty()) {
            return instanceName;
        }
        return Concat({instanceName, topicName}, "::");
    }
    std::string GetType() const
    {
        return Concat({instanceName, topicName, params}, "::");
    }
    static Topic GetTopicFromType(const std::string &type)
    {
        auto values = SplitString(type, "::");
        std::string newInstanceName = values[0];
        std::string newTopicName = values[1];
        std::string newParams = (values.size() > 2 ? values[2] : "");
        return Topic{newInstanceName, newTopicName, newParams};
    }
};


struct Result {
    int code;
    std::string payload;
    Result() { }
    explicit Result(int code) : code(code) { }
    Result(int code, const std::string &payload) : code(code), payload(payload) { }
    void Serialize(oeaware::OutStream &out) const
    {
        out << code << payload;
    }
    void Deserialize(oeaware::InStream &in)
    {
        in >> code >> payload;
    }
};
}

#endif

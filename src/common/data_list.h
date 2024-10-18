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
#ifndef COMMON_DATA_LIST_H
#define COMMON_DATA_LIST_H
#include "data_register.h"
#include "utils.h"

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
};

struct Result {
    int code;
    std::string payload;
    void Serialize(oeaware::OutStream &out) const
    {
        out << code << payload;
    }
    void Deserialize(oeaware::InStream &in)
    {
        in >> code >> payload;
    }
};

struct DataList {
    Topic topic;
    std::vector<std::shared_ptr<BaseData>> data;
    void Serialize(oeaware::OutStream &out) const
    {
        out << topic << data;
    }
    void Deserialize(oeaware::InStream &in)
    {
        in >> topic;
        auto type = topic.GetDataType();
        size_t len;
        in >> len;
        for (size_t i = 0; i < len; ++i) {
            auto realData = BaseData::Create(type);
            in >> realData;
            data.emplace_back(realData);
        }
    }
};
}
#endif

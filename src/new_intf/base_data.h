//
// Created by w00847030 on 2024/10/11.
//

#ifndef OEAWARE_BASE_DATA_H
#define OEAWARE_BASE_DATA_H
#include <functional>
#include "../common/serialize.h"

namespace std { // 放在这里？
    template <>
    struct hash<oeaware::Topic> {
        size_t operator()(const oeaware::Topic& topic) const {
            size_t h1 = std::hash<std::string>{}(topic.instanceName);
            size_t h2 = std::hash<std::string>{}(topic.topicName);
            size_t h3 = std::hash<std::string>{}(topic.params);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

namespace oeaware {
struct BaseData {  // 实际采样时间封装在这里
    virtual void Serialize(oeaware::OutStream &out) const = 0;
    virtual void Deserialize(oeaware::InStream &in) = 0;
    static void RegisterClass(const std::string &key, std::function<std::shared_ptr<BaseData>()> constructor);
    static void RegisterClass(const std::vector<std::string> &keys,
        std::function<std::shared_ptr<BaseData>()> constructor);
    static std::shared_ptr<BaseData> Create(const std::string &type);
};

struct Topic { // 要不要补充hash函数适配 unordered_map、set
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
    std::vector<BaseData *> data;
    void Serialize(oeaware::OutStream &out) const
    {
        //   out << topic << data;
    }
    void Deserialize(oeaware::InStream &in)
    {
        //        in >> topic;
        //        auto type = Hash({topic.instanceName, topic.topicName});
        //        size_t len;
        //        in >> len;
        //        for (size_t i = 0; i < len; ++i) {
        //            auto realData = BaseData::Create(type);
        //            in >> realData;
        //            data.emplace_back(realData);
        //        }
    }
    // 定义 operator==
    bool operator==(const Topic &other) const
    {
        return instanceName == other.instanceName &&
            topicName == other.topicName &&
            params == other.params;
    }
};
}
#endif //OEAWARE_BASE_DATA_H


#ifndef PMU_COLLECTOR_H
#define PMU_COLLECTOR_H
#include "new_intf.h"
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include "pmu.h"
#include "pcerrc.h"
#include <chrono>

namespace std {
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

class TopicProcessor {
public:
    virtual ~TopicProcessor();
    bool Open();
    void Close();
    void Run(int &len, PmuData **data, uint64_t &interval);
protected:
    virtual void SetPmuAttr() = 0;
    std::string topicName;
    int pmuId = -1;
    PmuAttr attr;
    //uint64_t interval = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
};

class PmuCount :public oeaware::Interface
{
public:
    PmuCount();
    ~PmuCount() noexcept(true) override = default;  // ?
    bool OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override; //
    void UpdateData(const oeaware::DataList &dataList) override; // pmu not use
    std::vector<std::string> GetSupportTopics() override; // vector ?
    bool Enable(const std::string &parma) override; // oeaware -e ins
    void Disable() override; // oeaware -d ins
    void Run() override;    // loop

private:
    std::unordered_map<std::string, std::unique_ptr<TopicProcessor>> topics {};
    std::vector<std::string> supportTopics{}; // 放在基类？
    std::unordered_set<oeaware::Topic> publishTopics; // 想用unordered_set 需要写hash函数
};

#endif
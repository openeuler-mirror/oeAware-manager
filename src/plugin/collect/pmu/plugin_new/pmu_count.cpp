#include "pmu_count.h"
#include <iostream>
#include "pmu_count_data.h"
oeaware::Register<PmuCountData> PmuCountData::pmuCountingReg("pmu_count");
static void InitPmuAttr(struct PmuAttr &attr) {
    attr.evtList = nullptr;
    attr.numEvt = 0;
    attr.pidList = nullptr;
    attr.numPid = 0;
    attr.cpuList = nullptr;
    attr.numCpu = 0;
    attr.evtAttr = nullptr;
    attr.period = 0;
    attr.useFreq = 0;
    attr.excludeUser = 0;
    attr.excludeKernel = 0;
    attr.symbolMode = NO_SYMBOL_RESOLVE;
    attr.callStack = 0;
    attr.dataFilter = SPE_FILTER_NONE;
    attr.evFilter = SPE_EVENT_NONE;
    attr.minLatency = 0;
    attr.includeNewFork = 0;
}

class BaseProc : public TopicProcessor {
protected:
    void SetPmuAttr() override
    {
        attr.evtList = new char *[1];
        attr.evtList[0] = new char[topicName.size() + 1];
        std::strcpy(attr.evtList[0], topicName.c_str());
        attr.period = 1;
        attr.numEvt = 1;
    }
};

class NetEventProc : public TopicProcessor {
protected:
    void SetPmuAttr() override {
        attr.evtList = new char *[1];
        attr.evtList[0] = new char[topicName.size() + 1];
        std::strcpy(attr.evtList[0], topicName.c_str());
        attr.period = 10;
        attr.numEvt = 1;
    }
};

//class UncoreProc : public TopicProcessor {
//protected:
//    void SetPmuAttr() override {
//        attr.evtList = { topicName.c_str() };
//        attr.numEvt = static_cast<int>(attr.evtList.size());
//    }
//};

TopicProcessor::~TopicProcessor()
{
}

bool TopicProcessor::Open() // open 的时候要不要直接enable，这样是否能保证第一个周期也有数据
{
    InitPmuAttr(attr);
    SetPmuAttr();
    pmuId = PmuOpen(COUNTING, &attr);
    if (pmuId == -1) {
        std::cout << "open pmu count failed, reason is: " << Perror() << std::endl;
    } else {
        timestamp = std::chrono::high_resolution_clock::now();
        PmuEnable(pmuId);
    }
    return pmuId != -1;
}

void TopicProcessor::Close()
{
    PmuClose(pmuId);
    pmuId = -1;
}

void TopicProcessor::Run(int &len, PmuData **data, uint64_t &interval)
{
    PmuDisable(pmuId);
    len = PmuRead(pmuId, data);
    PmuEnable(pmuId);
    auto now = std::chrono::high_resolution_clock::now();
    interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp).count();
    timestamp = now;
}


PmuCount::PmuCount()
{
    name = "pmu_count";
    description = "pmu count";
    version = "1.0.0";
    period = 100; // 100ms
    priority = 0;

    topics["cycles"] = std::make_unique<BaseProc>();
    topics["cache-misses"] = std::make_unique<BaseProc>();
    topics["net:netif_rx"] = std::make_unique<NetEventProc>();
//    topic["rx_outer"] = std::make_unique<UncoreProc>();

    for (auto &i : topics) {
        supportTopics.emplace_back(i.first);
    }
}

bool PmuCount::OpenTopic(const oeaware::Topic &topic)
{
    if (topics.find(topic.instanceName) == topics.end()) {
        return false;
    }
    if (!topics[topic.instanceName]->Open()) {
        return false;
    }
    publishTopics.insert(topic);
    return true;
}

void PmuCount::CloseTopic(const oeaware::Topic &topic)
{
    topics[topic.instanceName]->Close();
    publishTopics.erase(topic);
}

void PmuCount::UpdateData(const oeaware::DataList &dataList)
{
    (void)dataList;
}

std::vector<std::string> PmuCount::GetSupportTopics()
{
    return supportTopics;
}

bool PmuCount::Enable(const std::string &parma)
{
    (void)parma;
    return true;
}

void PmuCount::Disable()
{
    supportTopics.clear();
}

void PmuCount::Run()
{
    // BaseData 是基类， pmuCountData 是子类
    // DataList 的成员是 BaseData

    // 我的数据怎么定义？？时间放在哪里，
    for (auto &it : publishTopics) {
        oeaware::DataList dataList;
        dataList.topic.instanceName = name;
        dataList.topic.topicName = it.topicName;
        std::shared_ptr<PmuCountData> data = std::make_shared<PmuCountData>();
        topics[it.topicName]->Run(data->len, &data->pmuData, data->interval);
        dataList.data.emplace_back(data);
        Publish(dataList);
    }
}

void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<PmuCount>());
}

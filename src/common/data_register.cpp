#include "data_register.h"
#include <securec.h>
#include "data_list.h"
#include "utils.h"
#if defined(__arm__) || defined(__aarch64__)
#include "pmu_counting_data.h"
#include "pmu_sampling_data.h"
#include "pmu_spe_data.h"
#include "pmu_uncore_data.h"
#include "symbol.h"
#endif

namespace oeaware {

int TopicSerialize(const void *topic, OutStream &out)
{
    auto tmpTopic = static_cast<const CTopic*>(topic);
    std::string instanceName(tmpTopic->instanceName);
    std::string topicName(tmpTopic->topicName);
    std::string params(tmpTopic->params);
    out << instanceName << topicName << params;
    return 0;
}

int TopicDeserialize(void *topic, InStream &in)
{
   std::string instanceName;
   std::string topicName;
   std::string params;
   in >> instanceName >> topicName >> params;
   ((CTopic*)topic)->instanceName = new char[instanceName.size() + 1];
   ((CTopic*)topic)->topicName = new char[topicName.size() + 1];
   ((CTopic*)topic)->params = new char[params.size() + 1];
   
   auto ret = strcpy_s(((CTopic*)topic)->instanceName, instanceName.size() + 1, instanceName.data());
   if (ret != EOK) return ret;
   ret = strcpy_s(((CTopic*)topic)->topicName, topicName.size() + 1, topicName.data());
   if (ret != EOK) return ret;
   ret = strcpy_s(((CTopic*)topic)->params, params.size() + 1, params.data());
   if (ret != EOK) return ret;
   return 0;
}

int DataListSerialize(const void *dataList, OutStream &out)
{
    auto tmpList = static_cast<const DataList*>(dataList);
    TopicSerialize(&tmpList->topic, out);
    out << tmpList->len;
    auto &reg = Register::GetInstance();
    auto func = reg.GetDataSerialize(Concat({tmpList->topic.instanceName, tmpList->topic.topicName}, "::"));
     if (func == nullptr) {
        func = reg.GetDataSerialize(tmpList->topic.instanceName);
    }
    for (int i = 0; i < tmpList->len; ++i) {
        func(tmpList->data[i], out);
    }
    return 0;
}

int DataListDeserialize(void *dataList, InStream &in)
{
    CTopic topic;
    TopicDeserialize(&topic, in);
    uint64_t size;
    in >> size;
    ((DataList*)dataList)->topic = topic;
    ((DataList*)dataList)->len = size;
    ((DataList*)dataList)->data = new void* [size];
    auto &reg = Register::GetInstance();
    auto func = reg.GetDataDeserialize(Concat({topic.instanceName, topic.topicName}, "::"));
    if (func == nullptr) {
        func = reg.GetDataDeserialize(topic.instanceName);
    }
    for (int i = 0; i < size; ++i) {
        ((DataList*)dataList)->data[i] = nullptr;
        auto ret = func(&(((DataList*)dataList)->data[i]), in);
        if (!ret) {
            return ret;
        }
    }
    return 0;
}


int ResultDeserialize(void *data, InStream &in)
{
    auto tmpData = static_cast<Result*>(data);
    std::string msg;
    in >> tmpData->code >> msg;
    tmpData->payload = new char[msg.size() + 1];
    auto ret = strcpy_s(tmpData->payload, msg.size() + 1, msg.data());
    return ret;
}
#if defined(__arm__) || defined(__aarch64__)
int PmuCountingDataSerialize(const void *data, OutStream &out)
{
    auto tmpData = static_cast<const PmuCountingData*>(data);
    uint64_t interval = tmpData->interval;
    int len = tmpData->len;
    PmuData *pmuData = tmpData->pmuData;
    out << interval << len;
    for (int i = 0; i < len; i++) {
        int count = 0;
        auto tmp = pmuData[i].stack;
        while (tmp != nullptr) {
            count++;
            tmp = tmp->next;
        }
        out << count;
        tmp = pmuData[i].stack;
        while (count--) {
            std::string module(tmp->symbol->module);
            std::string symbolName(tmp->symbol->symbolName);
            std::string mangleName(tmp->symbol->mangleName);
            std::string fileName(tmp->symbol->fileName);
            out << tmp->symbol->addr << module << symbolName << mangleName << fileName
                << tmp->symbol->lineNum << tmp->symbol->offset << tmp->symbol->codeMapEndAddr
                << tmp->symbol->codeMapAddr << tmp->symbol->count << tmp->count;
            tmp = tmp->next;
        }
        out << pmuData[i].evt << pmuData[i].ts << pmuData[i].pid << pmuData[i].tid << pmuData[i].cpu
            << pmuData[i].cpuTopo->coreId << pmuData[i].cpuTopo->numaId << pmuData[i].cpuTopo->socketId
            << pmuData[i].comm << pmuData[i].period << pmuData[i].count << pmuData[i].countPercent;
    }
    return 0;
}

int PmuCountingDataDeserialize(void **data, InStream &in)
{
    *data = new PmuCountingData();
    int len;
    uint64_t interval;
    in >> interval >> len;
    ((PmuCountingData*)(*data))->len = len;
    ((PmuCountingData*)(*data))->interval = interval;
    PmuData *pmuData = new struct PmuData[len];
    for (int i = 0; i < len; i++) {
        int count;
        in >> count;
        pmuData[i].stack = new Stack();
        auto tmp = pmuData[i].stack;
        while (count--) {
            if (count) {
                tmp->next = new Stack();
            }
            tmp->symbol = new Symbol();
            std::string module, symbolName, mangleName, fileName;

            in >> tmp->symbol->addr >> module >> symbolName >> mangleName >> fileName
            >> tmp->symbol->lineNum >> tmp->symbol->offset >> tmp->symbol->codeMapEndAddr
            >> tmp->symbol->codeMapAddr >> tmp->symbol->count >> tmp->count;

            tmp->symbol->module = new char[module.length() + 1];
            errno_t ret = strcpy_s(tmp->symbol->module, module.length() + 1, module.c_str());
            if (ret != EOK) {
                return ret;
            }
            tmp->symbol->symbolName = new char[symbolName.length() + 1];
            ret = strcpy_s(tmp->symbol->symbolName, symbolName.length() + 1, symbolName.c_str());
            if (ret != EOK) {
                return ret;
            }
            tmp->symbol->mangleName = new char[mangleName.length() + 1];
            ret = strcpy_s(tmp->symbol->mangleName, mangleName.length() + 1, mangleName.c_str());
            if (ret != EOK) {
                return ret;
            }
            tmp->symbol->fileName = new char[fileName.length() + 1];
            ret = strcpy_s(tmp->symbol->fileName, fileName.length() + 1, fileName.c_str());
            if (ret != EOK) {
                return ret;
            }
            tmp = tmp->next;
        }
        pmuData[i].cpuTopo = new CpuTopology();
        in >> pmuData[i].evt >> pmuData[i].ts >> pmuData[i].pid >> pmuData[i].tid >> pmuData[i].cpu
        >> pmuData[i].cpuTopo->coreId >> pmuData[i].cpuTopo->numaId >> pmuData[i].cpuTopo->socketId
        >> pmuData[i].comm >> pmuData[i].period >> pmuData[i].count >> pmuData[i].countPercent;
    }
    ((PmuCountingData*)(*data))->pmuData = pmuData;
    return 0;
}

int PmuSamplingDataSerialize(const void *data, OutStream &out)
{
    auto tmpData = static_cast<const PmuSamplingData*>(data);
    uint64_t interval = tmpData->interval;
    int len = tmpData->len;
    PmuData *pmuData = tmpData->pmuData;
    out << interval << len;
    for (int i = 0; i < len; i++) {
        int count = 0;
        auto tmp = pmuData[i].stack;
        while (tmp != nullptr) {
            count++;
            tmp = tmp->next;
        }
        out << count;
        tmp = pmuData[i].stack;
        while (count--) {
            std::string module(tmp->symbol->module);
            std::string symbolName(tmp->symbol->symbolName);
            std::string mangleName(tmp->symbol->mangleName);
            std::string fileName(tmp->symbol->fileName);
            out << tmp->symbol->addr << module << symbolName << mangleName << fileName
                << tmp->symbol->lineNum << tmp->symbol->offset << tmp->symbol->codeMapEndAddr
                << tmp->symbol->codeMapAddr << tmp->symbol->count << tmp->count;
            tmp = tmp->next;
        }
        out << pmuData[i].evt << pmuData[i].ts << pmuData[i].pid << pmuData[i].tid << pmuData[i].cpu
            << pmuData[i].cpuTopo->coreId << pmuData[i].cpuTopo->numaId << pmuData[i].cpuTopo->socketId
            << pmuData[i].comm << pmuData[i].period;
    }
    return 0;
}

int PmuSamplingDataDeserialize(void **data, InStream &in)
{
    *data = new PmuSamplingData();
    int len;
    uint64_t interval;
    in >> interval >> len;
    ((PmuSamplingData*)(*data))->len = len;
    ((PmuSamplingData*)(*data))->interval = interval;
    PmuData *pmuData = new struct PmuData[len];
    for (int i = 0; i < len; i++) {
        int count;
        in >> count;
        pmuData[i].stack = new Stack();
        auto tmp = pmuData[i].stack;
        while (count--) {
            if (count) {
                tmp->next = new Stack();
            }
            tmp->symbol = new Symbol();
            std::string module, symbolName, mangleName, fileName;

            in >> tmp->symbol->addr >> module >> symbolName >> mangleName >> fileName
            >> tmp->symbol->lineNum >> tmp->symbol->offset >> tmp->symbol->codeMapEndAddr
            >> tmp->symbol->codeMapAddr >> tmp->symbol->count >> tmp->count;

            tmp->symbol->module = new char[module.length() + 1];
            errno_t ret = strcpy_s(tmp->symbol->module, module.length() + 1, module.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp->symbol->symbolName = new char[symbolName.length() + 1];
            ret = strcpy_s(tmp->symbol->symbolName, symbolName.length() + 1, symbolName.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp->symbol->mangleName = new char[mangleName.length() + 1];
            ret = strcpy_s(tmp->symbol->mangleName, mangleName.length() + 1, mangleName.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp->symbol->fileName = new char[fileName.length() + 1];
            ret = strcpy_s(tmp->symbol->fileName, fileName.length() + 1, fileName.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp = tmp->next;
        }
        pmuData[i].cpuTopo = new CpuTopology();
        in >> pmuData[i].evt >> pmuData[i].ts >> pmuData[i].pid >> pmuData[i].tid >> pmuData[i].cpu
        >> pmuData[i].cpuTopo->coreId >> pmuData[i].cpuTopo->numaId >> pmuData[i].cpuTopo->socketId
        >> pmuData[i].comm >> pmuData[i].period;
    }
    ((PmuSamplingData*)(*data))->pmuData = pmuData;
    return 0;
}

int PmuSpeDataSerialize(const void *data, OutStream &out)
{
    auto tmpData = static_cast<const PmuSpeData*>(data);
    uint64_t interval = tmpData->interval;
    int len = tmpData->len;
    PmuData *pmuData = tmpData->pmuData;
    out << interval << len;
    for (int i = 0; i < len; i++) {
        int count = 0;
        auto tmp = pmuData[i].stack;
        while (tmp != nullptr) {
            count++;
            tmp = tmp->next;
        }
        out << count;
        tmp = pmuData[i].stack;
        while (count--) {
            std::string module(tmp->symbol->module);
            std::string symbolName(tmp->symbol->symbolName);
            std::string mangleName(tmp->symbol->mangleName);
            std::string fileName(tmp->symbol->fileName);
            out << tmp->symbol->addr << module << symbolName << mangleName << fileName
                << tmp->symbol->lineNum << tmp->symbol->offset << tmp->symbol->codeMapEndAddr
                << tmp->symbol->codeMapAddr << tmp->symbol->count << tmp->count;
            tmp = tmp->next;
        }
        out << pmuData[i].evt << pmuData[i].ts << pmuData[i].pid << pmuData[i].tid << pmuData[i].cpu
            << pmuData[i].cpuTopo->coreId << pmuData[i].cpuTopo->numaId << pmuData[i].cpuTopo->socketId
            << pmuData[i].comm << pmuData[i].period << pmuData[i].ext->pa << pmuData[i].ext->va
            << pmuData[i].ext->event;
    }
    return 0;
}

int PmuSpeDataDeserialize(void **data, InStream &in)
{
    *data = new PmuSpeData();
    int len;
    uint64_t interval;
    in >> interval >> len;
    ((PmuSpeData*)(*data))->len = len;
    ((PmuSpeData*)(*data))->interval = interval;
    PmuData *pmuData = new struct PmuData[len];
    for (int i = 0; i < len; i++) {
        int count;
        in >> count;
        pmuData[i].stack = new Stack();
        auto tmp = pmuData[i].stack;
        while (count--) {
            if (count) {
                tmp->next = new Stack();
            }
            tmp->symbol = new Symbol();
            std::string module, symbolName, mangleName, fileName;

            in >> tmp->symbol->addr >> module >> symbolName >> mangleName >> fileName
            >> tmp->symbol->lineNum >> tmp->symbol->offset >> tmp->symbol->codeMapEndAddr
            >> tmp->symbol->codeMapAddr >> tmp->symbol->count >> tmp->count;

            tmp->symbol->module = new char[module.length() + 1];
            errno_t ret = strcpy_s(tmp->symbol->module, module.length() + 1, module.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp->symbol->symbolName = new char[symbolName.length() + 1];
            ret = strcpy_s(tmp->symbol->symbolName, symbolName.length() + 1, symbolName.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp->symbol->mangleName = new char[mangleName.length() + 1];
            ret = strcpy_s(tmp->symbol->mangleName, mangleName.length() + 1, mangleName.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp->symbol->fileName = new char[fileName.length() + 1];
            ret = strcpy_s(tmp->symbol->fileName, fileName.length() + 1, fileName.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp = tmp->next;
        }
        pmuData[i].cpuTopo = new CpuTopology();
        pmuData[i].ext = new PmuDataExt();
        in >> pmuData[i].evt >> pmuData[i].ts >> pmuData[i].pid >> pmuData[i].tid >> pmuData[i].cpu
        >> pmuData[i].cpuTopo->coreId >> pmuData[i].cpuTopo->numaId >> pmuData[i].cpuTopo->socketId
        >> pmuData[i].comm >> pmuData[i].period >> pmuData[i].ext->pa >> pmuData[i].ext->va
        >> pmuData[i].ext->event;
    }
    ((PmuSpeData*)(*data))->pmuData = pmuData;
    return 0;
}

int PmuUncoreDataSerialize(const void *data, OutStream &out)
{
    auto tmpData = static_cast<const PmuUncoreData*>(data);
    uint64_t interval = tmpData->interval;
    int len = tmpData->len;
    PmuData *pmuData = tmpData->pmuData;
    out << interval << len;
    for (int i = 0; i < len; i++) {
        int count = 0;
        auto tmp = pmuData[i].stack;
        while (tmp != nullptr) {
            count++;
            tmp = tmp->next;
        }
        out << count;
        tmp = pmuData[i].stack;
        while (count--) {
            std::string module(tmp->symbol->module);
            std::string symbolName(tmp->symbol->symbolName);
            std::string mangleName(tmp->symbol->mangleName);
            std::string fileName(tmp->symbol->fileName);
            out << tmp->symbol->addr << module << symbolName << mangleName << fileName
                << tmp->symbol->lineNum << tmp->symbol->offset << tmp->symbol->codeMapEndAddr
                << tmp->symbol->codeMapAddr << tmp->symbol->count << tmp->count;
            tmp = tmp->next;
        }
        out << pmuData[i].evt << pmuData[i].ts << pmuData[i].pid << pmuData[i].tid << pmuData[i].cpu
            << pmuData[i].cpuTopo->coreId << pmuData[i].cpuTopo->numaId << pmuData[i].cpuTopo->socketId
            << pmuData[i].comm << pmuData[i].period << pmuData[i].count << pmuData[i].countPercent;
    }
    return 0;
}

int PmuUncoreDataDeserialize(void **data, InStream &in)
{
    *data = new PmuUncoreData();
    int len;
    uint64_t interval;
    in >> interval >> len;
    ((PmuUncoreData*)(*data))->len = len;
    ((PmuUncoreData*)(*data))->interval = interval;
    PmuData *pmuData = new struct PmuData[len];
    for (int i = 0; i < len; i++) {
        int count;
        in >> count;
        pmuData[i].stack = new Stack();
        auto tmp = pmuData[i].stack;
        while (count--) {
            if (count) {
                tmp->next = new Stack();
            }
            tmp->symbol = new Symbol();
            std::string module, symbolName, mangleName, fileName;

            in >> tmp->symbol->addr >> module >> symbolName >> mangleName >> fileName
            >> tmp->symbol->lineNum >> tmp->symbol->offset >> tmp->symbol->codeMapEndAddr
            >> tmp->symbol->codeMapAddr >> tmp->symbol->count >> tmp->count;

            tmp->symbol->module = new char[module.length() + 1];
            errno_t ret = strcpy_s(tmp->symbol->module, module.length() + 1, module.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp->symbol->symbolName = new char[symbolName.length() + 1];
            ret = strcpy_s(tmp->symbol->symbolName, symbolName.length() + 1, symbolName.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp->symbol->mangleName = new char[mangleName.length() + 1];
            ret = strcpy_s(tmp->symbol->mangleName, mangleName.length() + 1, mangleName.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp->symbol->fileName = new char[fileName.length() + 1];
            ret = strcpy_s(tmp->symbol->fileName, fileName.length() + 1, fileName.c_str());
            if (ret != EOK) {
                return -1;
            }
            tmp = tmp->next;
        }
        pmuData[i].cpuTopo = new CpuTopology();
        in >> pmuData[i].evt >> pmuData[i].ts >> pmuData[i].pid >> pmuData[i].tid >> pmuData[i].cpu
        >> pmuData[i].cpuTopo->coreId >> pmuData[i].cpuTopo->numaId >> pmuData[i].cpuTopo->socketId
        >> pmuData[i].comm >> pmuData[i].period >> pmuData[i].count >> pmuData[i].countPercent;
    }
    ((PmuUncoreData*)(*data))->pmuData = pmuData;
    return 0;
}
#endif
void Register::RegisterData(const std::string &name, const std::pair<SerializeFunc, DeserializeFunc> &func)
{
    deserializeFuncs[name] = func;
}

void Register::InitRegisterData()
{
#if defined(__arm__) || defined(__aarch64__)
    RegisterData("pmu_counting_collector", std::make_pair(PmuCountingDataSerialize, PmuCountingDataDeserialize));

    RegisterData("pmu_sampling_collector", std::make_pair(PmuSamplingDataSerialize, PmuSamplingDataDeserialize));

    RegisterData("pmu_spe_collector", std::make_pair(PmuSpeDataSerialize, PmuSpeDataDeserialize));

    RegisterData("pmu_uncore_collector", std::make_pair(PmuUncoreDataSerialize, PmuUncoreDataDeserialize));
#endif
}

SerializeFunc Register::GetDataSerialize(const std::string &name)
{
    if (!deserializeFuncs.count(name)) {
        return nullptr;
    }
    return deserializeFuncs[name].first;
}

DeserializeFunc Register::GetDataDeserialize(const std::string &name)
{
    if (!deserializeFuncs.count(name)) {
        return nullptr;
    }
    return deserializeFuncs[name].second;
}
}

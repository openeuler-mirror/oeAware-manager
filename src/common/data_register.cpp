#include "data_register.h"
#include <securec.h>
#include "oeaware/utils.h"
#if defined(__arm__) || defined(__aarch64__)
#include "oeaware/data/pmu_counting_data.h"
#include "oeaware/data/pmu_sampling_data.h"
#include "oeaware/data/pmu_spe_data.h"
#include "oeaware/data/pmu_uncore_data.h"
#include "libkperf/symbol.h"
#endif
#include "oeaware/data/thread_info.h"
#include "oeaware/data/kernel_data.h"
#include "oeaware/data/command_data.h"
#include "oeaware/data/analysis_data.h"
#include "oeaware/data/env_data.h"
#include "oeaware/data/network_interface_data.h"

namespace oeaware {
void TopicFree(CTopic *topic)
{
    if (topic == nullptr) {
        return;
    }
    if (topic->instanceName != nullptr) {
        delete[] topic->instanceName;
        topic->instanceName = nullptr;
    }
    if (topic->topicName != nullptr) {
        delete[] topic->topicName;
        topic->topicName = nullptr;
    }
    if (topic->params != nullptr) {
        delete[] topic->params;
        topic->params = nullptr;
    }
}

int TopicSerialize(const CTopic *topic, OutStream &out)
{
    std::string instanceName(topic->instanceName);
    std::string topicName(topic->topicName);
    std::string params(topic->params);
    out << instanceName << topicName << params;
    return 0;
}

int TopicDeserialize(CTopic *topic, InStream &in)
{
   std::string instanceName;
   std::string topicName;
   std::string params;
   in >> instanceName >> topicName >> params;
   topic->instanceName = new char[instanceName.size() + 1];
   topic->topicName = new char[topicName.size() + 1];
   topic->params = new char[params.size() + 1];
   
   auto ret = strcpy_s(topic->instanceName, instanceName.size() + 1, instanceName.data());
   if (ret != EOK) return ret;
   ret = strcpy_s(topic->topicName, topicName.size() + 1, topicName.data());
   if (ret != EOK) return ret;
   ret = strcpy_s(topic->params, params.size() + 1, params.data());
   if (ret != EOK) return ret;
   return 0;
}

void DataListFree(DataList *dataList, bool flag)
{
    if (dataList == nullptr) {
        return;
    }
    if (flag) {
        auto &reg = Register::GetInstance();
        DataFreeFunc free = reg.GetDataFreeFunc(Concat({dataList->topic.instanceName, dataList->topic.topicName},
            "::"));
        if (free == nullptr) {
            free = reg.GetDataFreeFunc(dataList->topic.instanceName);
        }
        if (free != nullptr) {
            for (uint64_t i = 0; i < dataList->len; ++i) {
                free(dataList->data[i]);
            }
        }
    }
    TopicFree(&dataList->topic);
    if (dataList->data != nullptr) {
        delete[] dataList->data;
        dataList->data = nullptr;
    }
}

int DataListSerialize(const DataList *dataList, OutStream &out)
{
    TopicSerialize(&dataList->topic, out);
    out << dataList->len;
    auto &reg = Register::GetInstance();
    auto func = reg.GetDataSerialize(Concat({dataList->topic.instanceName, dataList->topic.topicName}, "::"));
    if (func == nullptr) {
        func = reg.GetDataSerialize(dataList->topic.instanceName);
    }
    for (uint64_t i = 0; i < dataList->len; ++i) {
        func(dataList->data[i], out);
    }
    return 0;
}

int DataListDeserialize(DataList *dataList, InStream &in)
{
    CTopic topic;
    TopicDeserialize(&topic, in);
    uint64_t size;
    in >> size;
    dataList->topic = topic;
    dataList->len = size;
    dataList->data = new void* [size];
    auto &reg = Register::GetInstance();
    auto func = reg.GetDataDeserialize(Concat({topic.instanceName, topic.topicName}, "::"));
    if (func == nullptr) {
        func = reg.GetDataDeserialize(topic.instanceName);
    }
    for (uint64_t i = 0; i < size; ++i) {
        dataList->data[i] = nullptr;
        auto ret = func(&(dataList->data[i]), in);
        if (ret) {
            return ret;
        }
    }
    return 0;
}

void ResultFree(Result *result)
{
    if (result == nullptr) {
        return;
    }
    if (result->payload != nullptr) {
        delete[] result->payload;
        result->payload = nullptr;
    }
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
void PmuBaseDataFree(void *data)
{
    auto tmpData = static_cast<PmuCountingData*>(data);
    if (tmpData == nullptr) {
        return;
    }
    PmuDataFree(tmpData->pmuData);
    tmpData->pmuData = nullptr;
    tmpData->len = 0;
    delete tmpData;
}

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

void ThreadInfoFree(void *data)
{
    auto threadInfo = static_cast<ThreadInfo*>(data);
    if (threadInfo == nullptr) {
        return;
    }
    if (threadInfo->name != nullptr) {
        delete[] threadInfo->name;
        threadInfo->name = nullptr;
    }
    delete threadInfo;
}

int ThreadInfoSerialize(const void *data, OutStream &out)
{
    auto threadInfo = static_cast<const ThreadInfo*>(data);
    out << threadInfo->pid << threadInfo->tid;
    std::string name(threadInfo->name);
    out << name;
    return 0;
}

int ThreadInfoDeserialize(void **data, InStream &in)
{
    *data = new ThreadInfo();
    auto threadInfo = static_cast<ThreadInfo*>(*data);
    std::string name;
    in >> threadInfo->pid >> threadInfo->tid >> name;
    threadInfo->name = new char[name.size() + 1];
    strcpy_s(threadInfo->name, name.size() + 1, name.data());
    return 0;
}

void KernelDataFree(void *data)
{
    auto tmpData = static_cast<const KernelData*>(data);
    if (tmpData == nullptr) {
        return;
    }
    KernelDataNode *node = tmpData->kernelData;
    for (int i = 0; i < tmpData->len; ++i) {
        auto tmp = node->next;
        if (node == nullptr) {
            break;
        }
        if (node->key != nullptr) {
            delete[] node->key;
            node->key = nullptr;
        }
        if (node->value != nullptr) {
            delete[] node->value;
            node->value = nullptr;
        }
        delete node;
        node = tmp;
    }
    delete tmpData;
}

int KernelDataSerialize(const void *data, OutStream &out)
{
    auto tmpData = static_cast<const KernelData*>(data);
    out << tmpData->len;
    auto node = tmpData->kernelData;
    for (int i = 0; i < tmpData->len; i++) {
        std::string key(node->key);
        std::string value(node->value);
        out << key << value;
        node = node->next;
    }
    return 0;
}

int KernelDataDeserialize(void **data, InStream &in)
{
    *data = new KernelData();
    KernelData *tmpData = static_cast<KernelData*>(*data);

    in >> tmpData->len;
    KernelDataNode *kernelDataNode = nullptr;
    for (int i = 0; i < tmpData->len; i++) {
        KernelDataNode *node = new KernelDataNode;
        std::string key, value;
        in >> key >> value;
        node->key = new char[key.length() + 1];
        errno_t ret = strcpy_s(node->key, key.length() + 1, key.c_str());
        if (ret != EOK) {
            return -1;
        }
         node->value = new char[value.size() + 1];
        ret = strcpy_s(node->value, value.length() + 1, value.c_str());
        if (ret != EOK) {
            return -1;
        }
        node->next = nullptr;
        if (tmpData->kernelData == NULL) {
            tmpData->kernelData = node;
            kernelDataNode = node;
        } else {
            kernelDataNode->next = node;
            kernelDataNode = kernelDataNode->next;
        }
    }
    return 0;
}

void CommandDataFree(void *data)
{
    CommandData *commandData = (CommandData*)data;
    if (commandData == nullptr) {
        return;
    }
    for (int i = 0; i < commandData->attrLen; ++i) {
        if (commandData->itemAttr[i] != nullptr) {
            delete[] commandData->itemAttr[i];
            commandData->itemAttr[i] = nullptr;
        }
    }
    if (commandData->items == nullptr) {
        delete commandData;
        return;
    }
    for (int i = 0; i < commandData->itemLen; ++i) {
        for (int j = 0; j < commandData->attrLen; ++j) {
            if (commandData->items[i].value[j] != nullptr) {
                delete[] commandData->items[i].value[j];
                commandData->items[i].value[j] = nullptr;
            }
        }
    }
    delete[] commandData->items;
    commandData->items = nullptr;
    
    delete commandData;
}

int CommandDataSerialize(const void *data, OutStream &out)
{
    auto commandData = (CommandData*)data;
    out << commandData->attrLen << commandData->itemLen;
    for (int i = 0; i < commandData->attrLen; ++i) {
        std::string attr(commandData->itemAttr[i]);
        out << attr;
    }
    for (int i = 0; i < commandData->itemLen; ++i) {
        for (int j = 0; j < commandData->attrLen; ++j) {
            std::string item(commandData->items[i].value[j]);
            out << item;
        }
    }
    return 0;
}

int CommandDataDeserialize(void **data, InStream &in)
{
    *data = new CommandData();
    auto sarData = static_cast<CommandData*>(*data);
    in >> sarData->attrLen >> sarData->itemLen;
    int ret;
    for (int i = 0; i < sarData->attrLen; ++i) {
        std::string attr;
        in >> attr;
        sarData->itemAttr[i] = new char[attr.size() + 1];
        ret = strcpy_s(sarData->itemAttr[i], attr.size() + 1, attr.data());
        if (ret != EOK) {
            return -1;
        }
    }
    sarData->items = new CommandIter[sarData->itemLen];
    for (int i = 0; i < sarData->itemLen; ++i) {
        for (int j = 0; j < sarData->attrLen; ++j) {
            std::string item;
            in >> item;
            sarData->items[i].value[j] = new char[item.size() + 1];
            ret = strcpy_s(sarData->items[i].value[j], item.size() + 1, item.data());
            if (ret != EOK) {
                return -1;
            }
        }
    }
    return 0;
}

void AnalysisDataFree(void *data)
{
    auto analysisData = static_cast<AnalysisReport*>(data);
    if (analysisData == nullptr) {
        return;
    }
    if (analysisData->data != nullptr) {
        delete[] analysisData->data;
        analysisData->data = nullptr;
    }
    delete analysisData;
    analysisData = nullptr;
}

int AnalysisDataSerialize(const void *data, OutStream &out)
{
    auto analysisData = static_cast<const AnalysisReport*>(data);
    out << analysisData->reportType;
    out << std::string(analysisData->data);
    return 0;
}

int AnalysisDataDeserialize(void **data, InStream &in)
{
    *data = new AnalysisReport();
    auto analysisData = static_cast<AnalysisReport *>(*data);
    in >> analysisData->reportType;
    std::string tmpData;
    in >> tmpData;
    analysisData->data = new char[tmpData.size() + 1];
    strcpy_s(analysisData->data, tmpData.size() + 1, tmpData.data());
    return 0;
}


int EnvStaticDataSerialize(const void *data, OutStream &out)
{
    auto envData = static_cast<const EnvStaticInfo *>(data);
    out << envData->numaNum;
    out << envData->cpuNumConfig;
    out << envData->pageSize;
    out << envData->pageMask;
    for (int i = 0; i < envData->cpuNumConfig; ++i) {
        out << envData->cpu2Node[i];
    }
    for (int i = 0; i < envData->numaNum; ++i) {
        for (int j = 0; j < envData->numaNum; ++j) {
            out << envData->numaDistance[i][j];
        }
    }
    return 0;
}

int EnvStaticDataDeserialize(void **data, InStream &in)
{
    *data = new EnvStaticInfo();
    auto envData = static_cast<EnvStaticInfo *>(*data);
    in >> envData->numaNum;
    in >> envData->cpuNumConfig;
    in >> envData->pageSize;
    in >> envData->pageMask;
    envData->cpu2Node = new int[envData->cpuNumConfig];
    for (int i = 0; i < envData->cpuNumConfig; ++i) {
        in >> envData->cpu2Node[i];
    }

    envData->numaDistance = new int *[envData->numaNum];
    for (int i = 0; i < envData->numaNum; ++i) {
        envData->numaDistance[i] = new int[envData->numaNum];
        for (int j = 0; j < envData->numaNum; ++j) {
            in >> envData->numaDistance[i][j];
        }
    }
    return 0;
}

int EnvRealTimeDataSerialize(const void *data, OutStream &out)
{
    auto envData = static_cast<const EnvRealTimeInfo *>(data);
    out << envData->dataReady;
    out << envData->cpuNumConfig;
    out << envData->cpuNumOnline;
    for (int i = 0; i < envData->cpuNumConfig; ++i) {
        out << envData->cpuOnLine[i];
    }
    return 0;
}

int EnvRealTimeDataDeserialize(void **data, InStream &in)
{
    *data = new EnvRealTimeInfo();
    auto envData = static_cast<EnvRealTimeInfo *>(*data);
    in >> envData->dataReady;
    in >> envData->cpuNumConfig;
    in >> envData->cpuNumOnline;
    envData->cpuOnLine = new int[envData->cpuNumConfig];
    for (int i = 0; i < envData->cpuNumConfig; ++i) {
        in >> envData->cpuOnLine[i];
    }
    return 0;
}

int EnvCpuUtilSerialize(const void *data, OutStream &out)
{
    auto envData = static_cast<const EnvCpuUtilParam *>(data);
    out << envData->dataReady;
    out << envData->cpuNumConfig;
    for (int cpu = 0; cpu < envData->cpuNumConfig + 1; ++cpu) {
        for (int type = 0; type < CPU_UTIL_TYPE_MAX; ++type) {
            out << envData->times[cpu][type];
        }
    }

    return 0;
}

int EnvCpuUtilDeserialize(void **data, InStream &in)
{
    *data = new EnvCpuUtilParam();
    auto envData = static_cast<EnvCpuUtilParam *>(*data);
    in >> envData->dataReady;
    in >> envData->cpuNumConfig;
    envData->times = new uint64_t *[envData->cpuNumConfig + 1];
    for (int cpu = 0; cpu < envData->cpuNumConfig + 1; ++cpu) {
        envData->times[cpu] = new uint64_t[CPU_UTIL_TYPE_MAX];
        for (int type = 0; type < CPU_UTIL_TYPE_MAX; ++type) {
            in >> envData->times[cpu][type];
        }
    }
    return 0;
}

static int DataItemDeserialize(DataItem *dataItem, int len, InStream &in)
{
    for (int i = 0; i < len; ++i) {
        std::string metric;
        std::string value;
        std::string extra;
        in >> dataItem[i].type >> metric >> value >> extra;
        dataItem[i].metric = new char[metric.size() + 1];
        strcpy_s(dataItem[i].metric, metric.size() + 1, metric.data());
        dataItem[i].value = new char[value.size() + 1];
        strcpy_s(dataItem[i].value, value.size() + 1, value.data());
        dataItem[i].extra = new char[extra.size() + 1];
        strcpy_s(dataItem[i].extra, extra.size() + 1, extra.data());
    }
    return 0;
}

static int SuggestionItemDeserialize(SuggestionItem *suggestionItem, InStream &in)
{
    std::string suggestion;
    std::string opt;
    std::string extra;
    in >> suggestion >> opt >> extra;
    suggestionItem->suggestion = new char[suggestion.size() + 1];
    strcpy_s(suggestionItem->suggestion, suggestion.size() + 1, suggestion.data());
    suggestionItem->opt = new char[opt.size() + 1];
    strcpy_s(suggestionItem->opt, opt.size() + 1, opt.data());
    suggestionItem->extra = new char[extra.size() + 1];
    strcpy_s(suggestionItem->extra, extra.size() + 1, extra.data());
    return 0;
}

int AnalysisResultItemSerialize(const void *data, OutStream &out)
{
    auto analysisResultItem = static_cast<const AnalysisResultItem*>(data);
    out << analysisResultItem->dataItemLen;
    for (int i = 0; i < analysisResultItem->dataItemLen; ++i) {
        auto dataItem = analysisResultItem->dataItem[i];
        out << dataItem.type << std::string(dataItem.metric) << std::string(dataItem.value) << std::string(dataItem.extra);
    }
    out << std::string(analysisResultItem->conclusion);

    auto &suggestionItem = analysisResultItem->suggestionItem;
    out << std::string(suggestionItem.suggestion) << std::string(suggestionItem.opt) <<
        std::string(suggestionItem.extra);
    return 0;
}

int AnalysisResultItemDeserialize(void **data, InStream &in)
{
    *data = new AnalysisResultItem();
    auto analysisResultItem = static_cast<AnalysisResultItem*>(*data);
    
    in >> analysisResultItem->dataItemLen;
    analysisResultItem->dataItem = new DataItem[analysisResultItem->dataItemLen];
    DataItemDeserialize(analysisResultItem->dataItem, analysisResultItem->dataItemLen, in);
    std::string conclusion;
    in >> conclusion;
    analysisResultItem->conclusion = new char[conclusion.size() + 1];
    strcpy_s(analysisResultItem->conclusion, conclusion.size() + 1, conclusion.data());
    SuggestionItemDeserialize(&analysisResultItem->suggestionItem, in);
    return 0;
}

void EnvCpuUtilFree(void *data)
{
    // to do
}

int NetIntfBaseSerialize(const void *data, OutStream &out)
{
    auto netData = static_cast<const NetIntfBaseDataList *>(data);
    out << netData->count;
    for (int n = 0; n < netData->count; ++n) {
        out << std::string(netData->base[n].name);
        out << netData->base[n].operstate;
    }
    return 0;
}

int NetIntfBaseDeserialize(void **data, InStream &in)
{
    *data = new NetIntfBaseDataList();
    auto netData = static_cast<NetIntfBaseDataList *>(*data);
    in >> netData->count;
    if (netData->count <= 0) {
        return 0;
    }
    netData->base = new NetworkInterfaceData[netData->count];
    for (int n = 0; n < netData->count; ++n) {
        std::string name = "";
        in >> name;
        std::size_t length = std::min(name.size(), sizeof(netData->base[n].name) - 1);
        std::copy(name.begin(), name.begin() + length, netData->base[n].name);
        netData->base[n].name[length] = '\0';
        in >> netData->base[n].operstate;
    }
    return 0;
}

void NetIntfBaseFree(void *data)
{
    auto netData = static_cast<NetIntfBaseDataList*>(data);
    if (netData == nullptr) {
        return;
    }
    if (netData->base != nullptr) {
        delete[] netData->base;
        netData->base = nullptr;
    }
    netData->count = 0;
    delete netData;
    netData = nullptr;
}

int NetIntfDriverSerialize(const void *data, OutStream &out)
{
    auto netData = static_cast<const NetIntfDriverDataList *>(data);
    out << netData->count;
    for (int n = 0; n < netData->count; ++n) {
        out << std::string(netData->driver[n].name);
        out << std::string(netData->driver[n].driver);
        out << std::string(netData->driver[n].version);
        out << std::string(netData->driver[n].fwVersion);
        out << std::string(netData->driver[n].busInfo);
    }
    return 0;
}

static void CopyStringToCharArray(const std::string &source, char *dest, size_t destSize)
{
    std::size_t length = std::min(source.size(), destSize - 1);
    std::copy(source.begin(), source.begin() + length, dest);
    dest[length] = '\0';
}

int NetIntfDriverDeserialize(void **data, InStream &in)
{
    *data = new NetIntfDriverDataList();
    auto netData = static_cast<NetIntfDriverDataList *>(*data);
    in >> netData->count;
    if (netData->count <= 0) {
        return 0;
    }
    netData->driver = new NetworkInterfaceDriverData[netData->count];
    for (int n = 0; n < netData->count; ++n) {
        std::string name = "";
        in >> name;
        CopyStringToCharArray(name, netData->driver[n].name, sizeof(netData->driver[n].name));
        std::string driver = "";
        in >> driver;
        CopyStringToCharArray(driver, netData->driver[n].driver, sizeof(netData->driver[n].driver));
        std::string version = "";
        in >> version;
        CopyStringToCharArray(version, netData->driver[n].version, sizeof(netData->driver[n].version));
        std::string fwVersion = "";
        in >> fwVersion;
        CopyStringToCharArray(fwVersion, netData->driver[n].fwVersion, sizeof(netData->driver[n].fwVersion));
        std::string busInfo = "";
        in >> busInfo;
        CopyStringToCharArray(busInfo, netData->driver[n].busInfo, sizeof(netData->driver[n].busInfo));
    }
    return 0;
}

void NetIntfDriverFree(void *data)
{
    auto netData = static_cast<NetIntfDriverDataList*>(data);
    if (netData == nullptr) {
        return;
    }
    if (netData->driver != nullptr) {
        delete[] netData->driver;
        netData->driver = nullptr;
    }
    netData->count = 0;
    delete netData;
    netData = nullptr;
}

int NetLocalAffiSerialize(const void *data, OutStream &out)
{
    auto netData = static_cast<const ProcessNetAffinityDataList *>(data);
    out << netData->count;
    for (int n = 0; n < netData->count; ++n) {
        out << netData->affinity[n].pid1;
        out << netData->affinity[n].pid2;
        out << netData->affinity[n].level;
    }
    return 0;
}

int NetLocalAffiDeserialize(void **data, InStream &in)
{
    *data = new ProcessNetAffinityDataList();
    auto netData = static_cast<ProcessNetAffinityDataList *>(*data);
    in >> netData->count;
    if (netData->count <= 0) {
        return 0;
    }
    netData->affinity = new ProcessNetAffinityData[netData->count];
    for (int n = 0; n < netData->count; ++n) {
        in >> netData->affinity[n].pid1;
        in >> netData->affinity[n].pid2;
        in >> netData->affinity[n].level;
    }
    return 0;
}

void NetLocalAffiFree(void *data)
{
    auto netData = static_cast<ProcessNetAffinityDataList*>(data);
    if (netData == nullptr) {
        return;
    }
    if (netData->affinity != nullptr) {
        delete[] netData->affinity;
        netData->affinity = nullptr;
    }
    netData->count = 0;
    delete netData;
    netData = nullptr;
}

void Register::RegisterData(const std::string &name, const RegisterEntry &entry)
{
    registerEntry[name] = entry;
}

void Register::InitRegisterData()
{
#if defined(__arm__) || defined(__aarch64__)
    RegisterData("pmu_counting_collector", RegisterEntry(PmuCountingDataSerialize, PmuCountingDataDeserialize,
        PmuBaseDataFree));

    RegisterData("pmu_sampling_collector", RegisterEntry(PmuSamplingDataSerialize, PmuSamplingDataDeserialize,
        PmuBaseDataFree));

    RegisterData("pmu_spe_collector", RegisterEntry(PmuSpeDataSerialize, PmuSpeDataDeserialize, PmuBaseDataFree));

    RegisterData("pmu_uncore_collector", RegisterEntry(PmuUncoreDataSerialize, PmuUncoreDataDeserialize,
        PmuBaseDataFree));
    RegisterData("smc_d_analysis", RegisterEntry(AnalysisResultItemSerialize, AnalysisResultItemDeserialize));
    RegisterData("hugepage_analysis", RegisterEntry(AnalysisResultItemSerialize, AnalysisResultItemDeserialize));
    RegisterData("dynamic_smt_analysis", RegisterEntry(AnalysisResultItemSerialize, AnalysisResultItemDeserialize));
    RegisterData("xcall_analysis", RegisterEntry(AnalysisResultItemSerialize, AnalysisResultItemDeserialize));
    RegisterData(OE_NET_HIRQ_ANALYSIS, RegisterEntry(AnalysisResultItemSerialize, AnalysisResultItemDeserialize));
    RegisterData("numa_analysis", RegisterEntry(AnalysisResultItemSerialize, AnalysisResultItemDeserialize));
#endif
    RegisterData("thread_collector", RegisterEntry(ThreadInfoSerialize, ThreadInfoDeserialize, ThreadInfoFree));
    RegisterData("kernel_config", RegisterEntry(KernelDataSerialize, KernelDataDeserialize, KernelDataFree));
    RegisterData("thread_scenario", RegisterEntry(ThreadInfoSerialize, ThreadInfoDeserialize, ThreadInfoFree));
    RegisterData("command_collector", RegisterEntry(CommandDataSerialize, CommandDataDeserialize, CommandDataFree));
    RegisterData("env_info_collector::static", RegisterEntry(EnvStaticDataSerialize, EnvStaticDataDeserialize));
    RegisterData("env_info_collector::realtime", RegisterEntry(EnvRealTimeDataSerialize, EnvRealTimeDataDeserialize));
    RegisterData("env_info_collector::cpu_util", RegisterEntry(EnvCpuUtilSerialize, EnvCpuUtilDeserialize, EnvCpuUtilFree));
    std::string name = std::string(OE_NET_INTF_INFO) + std::string("::") + std::string(OE_NETWORK_INTERFACE_BASE_TOPIC);
    RegisterData(name, RegisterEntry(NetIntfBaseSerialize, NetIntfBaseDeserialize, NetIntfBaseFree));
    name = std::string(OE_NET_INTF_INFO) + std::string("::") + std::string(OE_NETWORK_INTERFACE_DRIVER_TOPIC);
    RegisterData(name, RegisterEntry(NetIntfDriverSerialize, NetIntfDriverDeserialize, NetIntfDriverFree));
    name = std::string(OE_NET_INTF_INFO) + std::string("::") + std::string(OE_LOCAL_NET_AFFINITY);
    RegisterData(name, RegisterEntry(NetLocalAffiSerialize, NetLocalAffiDeserialize, NetLocalAffiFree));
}

SerializeFunc Register::GetDataSerialize(const std::string &name)
{
    if (!registerEntry.count(name)) {
        return nullptr;
    }
    return registerEntry[name].se;
}

DeserializeFunc Register::GetDataDeserialize(const std::string &name)
{
    if (!registerEntry.count(name)) {
        return nullptr;
    }
    return registerEntry[name].de;
}

DataFreeFunc Register::GetDataFreeFunc(const std::string &name)
{
    if (!registerEntry.count(name)) {
        return nullptr;
    }
    return registerEntry[name].free;
}
}


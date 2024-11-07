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
#include "thread_info.h"
#include "kernel_data.h"
#include "command_data.h"

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
        if (ret) {
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

int MpstatDataSerialize(const void *data, OutStream &out)
{
    auto tmpData = static_cast<const MpstatData*>(data);
    out << tmpData->len;
    for (int i = 0; i < tmpData->len; i++) {
        std::string key(tmpData->mpstatArray[i].key);
        out << key << tmpData->mpstatArray[i].value;
    }
    return 0;
}

int MpstatDataDeserialize(void **data, InStream &in)
{
    *data = new MpstatData();
    auto *tmpData = static_cast<MpstatData*>(*data);
    in >> tmpData->len;
    tmpData->mpstatArray = new CommandArray[tmpData->len];

    for (int i = 0; i < tmpData->len; i++) {
        std::string key;
        in >> key >> tmpData->mpstatArray[i].value;

        tmpData->mpstatArray[i].key = new char[key.length() + 1];
        errno_t ret = strcpy_s(tmpData->mpstatArray[i].key, key.length() + 1, key.c_str());
        if (ret != EOK) {
            for (int j = 0; j < i; j++) {
                delete[] tmpData->mpstatArray[j].key;
            }
            delete[] tmpData->mpstatArray;
            delete tmpData;
            *data = nullptr;
            return -1;
        }
    }

    return 0;
}

int IostatDataSerialize(const void *data, OutStream &out)
{
    auto tmpData = static_cast<const IostatData*>(data);
    out << tmpData->len;
    for (int i = 0; i < tmpData->len; i++) {
        std::string key(tmpData->iostatArray[i].key);
        out << key << tmpData->iostatArray[i].value;
    }
    return 0;
}

int IostatDataDeserialize(void **data, InStream &in)
{
    *data = new IostatData();
    auto *tmpData = static_cast<IostatData*>(*data);
    in >> tmpData->len;
    tmpData->iostatArray = new CommandArray[tmpData->len];

    for (int i = 0; i < tmpData->len; i++) {
        std::string key;
        in >> key >> tmpData->iostatArray[i].value;

        tmpData->iostatArray[i].key = new char[key.length() + 1];
        errno_t ret = strcpy_s(tmpData->iostatArray[i].key, key.length() + 1, key.c_str());
        if (ret != EOK) {
            for (int j = 0; j < i; j++) {
                delete[] tmpData->iostatArray[j].key;
            }
            delete[] tmpData->iostatArray;
            delete tmpData;
            *data = nullptr;
            return -1;
        }
    }

    return 0;
}

int VmstatDataSerialize(const void *data, OutStream &out)
{
    auto tmpData = static_cast<const VmstatData*>(data);
    out << tmpData->len;
    for (int i = 0; i < tmpData->len; i++) {
        std::string key(tmpData->vmstatArray[i].key);
        out << key << tmpData->vmstatArray[i].value;
    }
    return 0;
}

int VmstatDataDeserialize(void **data, InStream &in)
{
    *data = new VmstatData();
    auto *tmpData = static_cast<VmstatData*>(*data);
    in >> tmpData->len;
    tmpData->vmstatArray = new CommandArray[tmpData->len];

    for (int i = 0; i < tmpData->len; i++) {
        std::string key;
        in >> key >> tmpData->vmstatArray[i].value;

        tmpData->vmstatArray[i].key = new char[key.length() + 1];
        errno_t ret = strcpy_s(tmpData->vmstatArray[i].key, key.length() + 1, key.c_str());
        if (ret != EOK) {
            for (int j = 0; j < i; j++) {
                delete[] tmpData->vmstatArray[j].key;
            }
            delete[] tmpData->vmstatArray;
            delete tmpData;
            *data = nullptr;
            return -1;
        }
    }

    return 0;
}

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
    RegisterData("thread_collector", std::make_pair(ThreadInfoSerialize, ThreadInfoDeserialize));

    RegisterData("kernel_config::get_kernel_config", std::make_pair(KernelDataSerialize, KernelDataDeserialize));

    RegisterData("command_collector::mpstat", std::make_pair(MpstatDataSerialize, MpstatDataDeserialize));

    RegisterData("command_collector::iostat", std::make_pair(IostatDataSerialize, IostatDataDeserialize));

    RegisterData("command_collector::vmstat", std::make_pair(VmstatDataSerialize, VmstatDataDeserialize));
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

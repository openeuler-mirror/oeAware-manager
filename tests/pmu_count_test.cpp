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
#include <gtest/gtest.h>
#include "data_register.h"

struct Symbol {
    unsigned long addr;    // address (dynamic allocated) of this symbol
    char* module;          // binary name of which the symbol belongs to
    char* symbolName;      // name of the symbol with demangle
    char* mangleName;      // name of the symbol with no demangle
    char* fileName;        // corresponding file of current symbol
    unsigned int lineNum;  // line number of a symbol in the file
    unsigned long offset;
    unsigned long codeMapEndAddr;  // function end address
    unsigned long codeMapAddr;     // real srcAddr of Asm Code or
    __u64 count;
};

struct Stack {
    struct Symbol* symbol;  // symbol info for current stack
    struct Stack* next;     // points to next position in stack
    struct Stack* prev;     // points to previous position in stack
    __u64 count;
} __attribute__((aligned(64)));

struct CpuTopology {
    int coreId;
    int numaId;
    int socketId;
};

struct PmuData {
    struct Stack* stack;            // call stack
    const char *evt;                // event name
    int64_t ts;                     // time stamp
    pid_t pid;                      // process id
    int tid;                        // thread id
    unsigned cpu;                   // cpu id
    struct CpuTopology *cpuTopo;    // cpu topology
    const char *comm;               // process command
    uint64_t period;                // sample period
    uint64_t count;                 // event count. Only available for Counting.
    double countPercent;            // event count Percent. when count = 0, countPercent = -1; Only available for Counting.
    struct PmuDataExt *ext;         // extension. Only available for Spe.
    struct SampleRawData *rawData;  // trace pointer collect data.
};

struct CountingData : public oeaware::BaseData {
    struct PmuData *pmuData = NULL;
    int len;
    static oeaware::Register<CountingData> pmuCountingReg;
    void Serialize(oeaware::OutStream &out) const
    {
        out << len;
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
    }
    void Deserialize(oeaware::InStream &in)
    {
        in >> len;
        pmuData = new struct PmuData[len];
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
                strcpy(tmp->symbol->module, module.c_str());
                tmp->symbol->symbolName = new char[symbolName.length() + 1];
                strcpy(tmp->symbol->symbolName, symbolName.c_str());
                tmp->symbol->mangleName = new char[mangleName.length() + 1];
                strcpy(tmp->symbol->mangleName, mangleName.c_str());
                tmp->symbol->fileName = new char[fileName.length() + 1];
                strcpy(tmp->symbol->fileName, fileName.c_str());
                tmp = tmp->next;
            }
            pmuData[i].cpuTopo = new CpuTopology();
            in >> pmuData[i].evt >> pmuData[i].ts >> pmuData[i].pid >> pmuData[i].tid >> pmuData[i].cpu
            >> pmuData[i].cpuTopo->coreId >> pmuData[i].cpuTopo->numaId >> pmuData[i].cpuTopo->socketId
            >> pmuData[i].comm >> pmuData[i].period >> pmuData[i].count >> pmuData[i].countPercent;
        }
    }
};

oeaware::Register<CountingData> CountingData::pmuCountingReg("pmu_counting_collector");

CountingData CreateCountingData(int n, int len)
{
    struct CountingData data;
    data.len = len;
    data.pmuData = new struct PmuData[data.len];
    for (int j = 0; j < len; j++) {
        data.pmuData[j].stack = new Stack();
        auto p = data.pmuData[j].stack;
        for (unsigned int i = 0; i < n; ++i) {
            unsigned long addr = 0xfff + i;
            unsigned int lineNum = 10 + i;
            unsigned long offset = 0x4f + i;
            unsigned long codeMapAddr = 0x3fff;
            unsigned long codeMapEndAddr = codeMapAddr + i;
            __u64 count = 0xfffff;
            char *module = (char*)"module1";
            char *symbolName = (char*)"s1";
            char *mangleName = (char*)"m1";
            char *fileName = (char*)"f1";
            struct Symbol *symbol = new Symbol{addr, module, symbolName, mangleName, fileName, lineNum, offset, codeMapEndAddr, codeMapAddr, count};
            p->symbol = symbol;
            p->count = i;
            if (i < n - 1) {
                p->next = new Stack();
            }
            p = p->next;
        }
        data.pmuData[j].evt = "evt_test";
        data.pmuData[j].comm = "comm_test";
        int coreId = 95;
        int numaId = 3;
        int sockId = 1;
        data.pmuData[j].cpu = j;
        data.pmuData[j].cpuTopo = new CpuTopology{coreId, numaId, sockId};
        data.pmuData[j].ts = 0xfffffff;
        data.pmuData[j].pid = 10;
        data.pmuData[j].tid = 11;
        data.pmuData[j].period = 0xff12;
        data.pmuData[j].count = j;
        data.pmuData[j].countPercent = 1;
    }
    return data;
}

TEST(Serialize, CountingData)
{
    int n = 10;
    int len = 10;
    CountingData data = CreateCountingData(n, len);
    oeaware::OutStream out;
    data.Serialize(out);
    oeaware::InStream in(out.Str());
    CountingData data1;
    data1.Deserialize(in);

    for (int j = 0; j < len; j++) {
        EXPECT_EQ(data1.len, data.len);
        auto tmpStack = data1.pmuData[j].stack;
        auto stack = data.pmuData[j].stack;
        for (int i = 0; i < n; ++i) {
            EXPECT_EQ(tmpStack->count, stack->count);
            EXPECT_EQ(tmpStack->symbol->addr, stack->symbol->addr);
            EXPECT_EQ(0, strcmp(tmpStack->symbol->module, stack->symbol->module));
            EXPECT_EQ(0, strcmp(tmpStack->symbol->fileName, stack->symbol->fileName));
            EXPECT_EQ(0, strcmp(tmpStack->symbol->symbolName, stack->symbol->symbolName));
            EXPECT_EQ(0, strcmp(tmpStack->symbol->mangleName, stack->symbol->mangleName));
            EXPECT_EQ(tmpStack->symbol->lineNum, stack->symbol->lineNum);
            EXPECT_EQ(tmpStack->symbol->offset, stack->symbol->offset);
            EXPECT_EQ(tmpStack->symbol->codeMapAddr, stack->symbol->codeMapAddr);
            EXPECT_EQ(tmpStack->symbol->codeMapEndAddr, stack->symbol->codeMapEndAddr);
            EXPECT_EQ(tmpStack->symbol->count, stack->symbol->count);
            tmpStack = tmpStack->next;
            stack = stack->next;
        }
        EXPECT_EQ(0, strcmp(data1.pmuData[j].evt, data.pmuData[j].evt));
        EXPECT_EQ(data1.pmuData[j].ts, data.pmuData[j].ts);
        EXPECT_EQ(data1.pmuData[j].pid, data.pmuData[j].pid);
        EXPECT_EQ(data1.pmuData[j].tid, data.pmuData[j].tid);
        EXPECT_EQ(data1.pmuData[j].cpu, data.pmuData[j].cpu);
        EXPECT_EQ(data1.pmuData[j].cpuTopo->coreId, data.pmuData[j].cpuTopo->coreId);
        EXPECT_EQ(data1.pmuData[j].cpuTopo->socketId, data.pmuData[j].cpuTopo->socketId);
        EXPECT_EQ(data1.pmuData[j].cpuTopo->numaId, data.pmuData[j].cpuTopo->numaId);
        EXPECT_EQ(0, strcmp(data1.pmuData[j].comm, data.pmuData[j].comm));
        EXPECT_EQ(data1.pmuData[j].period, data.pmuData[j].period);
        EXPECT_EQ(data1.pmuData[j].count, data.pmuData[j].count);
        EXPECT_EQ(data1.pmuData[j].countPercent, data.pmuData[j].countPercent);
    }
}

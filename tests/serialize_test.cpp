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
#include "serialize.h"

struct TestData {
    int a;
    long long b;
    char ch;
    std::vector<int> c;
    std::string name;
    TestData() { }
    TestData(int a, long long b, char ch, const std::vector<int> &c, const std::string &name) : a(a), b(b), ch(ch),
        c(c), name(name) { }
    void Serialize(oeaware::OutStream &out) const
    {
        out << a << b << ch << c << name;
    }
    void Deserialize(oeaware::InStream &in)
    {
        in >> a >> b >> ch >> c >> name;
    }
};

struct PmuDataExt {
    unsigned long pa;
    unsigned long va;
    unsigned long event;
    void Serialize(oeaware::OutStream &out) const
    {
        out << pa << va << event;
    }
    void Deserialize(oeaware::InStream &in)
    {
        in >> pa >> va >> event;
    }
};

struct CpuTopology {
    int coreId;
    int numaId;
    int socketId;
    void Serialize(oeaware::OutStream &out) const
    {
        out << coreId << numaId << socketId;
    }
    void Deserialize(oeaware::InStream &in)
    {
        in >> coreId >> numaId >> socketId;
    }
};

struct Symbol {
    unsigned long addr;    // address (dynamic allocated) of this symbol
    const char* module;          // binary name of which the symbol belongs to
    const char* symbolName;      // name of the symbol
    const char* fileName;        // corresponding file of current symbol
    unsigned int lineNum;  // line number of a symbol in the file
    unsigned long offset;
    unsigned long codeMapEndAddr;  // function end address
    unsigned long codeMapAddr;     // real srcAddr of Asm Code or
    __u64 count;
    void Serialize(oeaware::OutStream &out) const
    {
        out << addr << module << symbolName << fileName << lineNum << offset << codeMapEndAddr << codeMapAddr << count;
    }
    void Deserialize(oeaware::InStream &in)
    {
        in >> addr >> module >> symbolName >> fileName >> lineNum >> offset >> codeMapEndAddr >> codeMapAddr >> count;
    }
};

struct Stack {
    struct Symbol* symbol;  // symbol info for current stack
    struct Stack* next;     // points to next position in stack
    __u64 count;
} __attribute__((aligned(64)));

struct SpeData {
    struct Stack* stack;            // call stack
    const char *evt;                // event name
    int64_t ts;                     // time stamp
    pid_t pid;                      // process id
    int tid;                        // thread id
    unsigned cpu;                   // cpu id
    struct CpuTopology *cpuTopo;    // cpu topology
    const char *comm;               // process command
    uint64_t period;                // sample period
    struct PmuDataExt *ext;         // extension. Only available for Spe.
    void Serialize(oeaware::OutStream &out) const
    {
        auto p = stack;
        int count = 0;
        while (p != nullptr) {
            count++;
            p = p->next;
        }
        out << count;
        
        auto tmp = stack;
        while (count--) {
            out << *tmp->symbol;
            out << tmp->count;
            tmp = tmp->next;
        }
        out << evt << ts << pid << tid << cpu << *cpuTopo << comm << period << *ext;
    }
    void Deserialize(oeaware::InStream &in)
    {
        int count;
        in >> count;
        stack = new Stack();
        auto tmp = stack;
        while (count--) {
            if (count) {
                tmp->next = new Stack();
            }
            tmp->symbol = new Symbol();
            in >> *tmp->symbol;
            in >> tmp->count;
            tmp = tmp->next;
        }
        cpuTopo = new CpuTopology();
        ext = new PmuDataExt();
        in >> evt >> ts >> pid >> tid >> cpu >> *cpuTopo >> comm >> period >> *ext;
    }
};


TEST(Serialize, base_type)
{
    int aI32 = 10;
    short aShort = 11;
    long long aI64 = __LONG_LONG_MAX__;
    char aChar = 'a';
    std::string aString = "hello world";
    oeaware::OutStream out;
    out << aShort << aI32 << aI64 << aChar << aString;
    oeaware::InStream in(out.Str());
    int bI32;
    short bShort;
    long long bI64;
    char bChar;
    std::string bString;
    in >> bShort >> bI32 >> bI64 >> bChar >> bString;
    EXPECT_EQ(bShort, aShort);
    EXPECT_EQ(bI32, aI32);
    EXPECT_EQ(bI64, aI64);
    EXPECT_EQ(bChar, aChar);
    EXPECT_EQ(bString, aString);
}

TEST(Serialize, vector)
{
    std::vector<int> a;
    for (int i = 0; i < 10; ++i) {
        a.emplace_back(i);
    }
    oeaware::OutStream out;
    out << a;
    oeaware::InStream in(out.Str());
    std::vector<int> b;
    in >> b;
    EXPECT_EQ(b.size(), a.size());
    for (int i = 0; i < b.size(); ++i) {
        EXPECT_EQ(b[i], a[i]);
    }
}

TEST(Serialize, smart_pointer)
{
    std::shared_ptr<int> a = std::make_shared<int>(10);
    oeaware::OutStream out;
    out << a;
    oeaware::InStream in(out.Str());
    std::shared_ptr<int> b = std::make_shared<int>();
    in >> b;
    EXPECT_EQ(*a, *b);
}

TEST(Serialize, custom_struct)
{
    TestData data(1, __LONG_LONG_MAX__, 'a', {1, 2, 3}, "hello world");
    oeaware::OutStream out;
    out << data;
    TestData dataFromStream;
    oeaware::InStream in(out.Str());
    in >> dataFromStream;
    EXPECT_EQ(dataFromStream.a, data.a);
    EXPECT_EQ(dataFromStream.b, data.b);
    EXPECT_EQ(dataFromStream.ch, data.ch);
    EXPECT_EQ(dataFromStream.c.size(), data.c.size());
    for (int i = 0; i < dataFromStream.c.size(); ++i) {
        EXPECT_EQ(dataFromStream.c[i], data.c[i]);
    }
    EXPECT_EQ(dataFromStream.name, data.name);
}


SpeData CreateSpeData(int n)
{
    Stack *stack = new Stack();
    auto p = stack;
    
    for (unsigned int i = 0; i < n; ++i) {
        unsigned long addr = 0xfff + i;
        unsigned int lineNum = 10 + i;
        unsigned long offset = 0x4f + i;
        unsigned long codeMapAddr = 0x3fff;
        unsigned long codeMapEndAddr = codeMapAddr + i;
        __u64 count = 0xfffff;
        Symbol *sympol = new Symbol{addr, "module1", "s1", "a1", lineNum, offset, codeMapEndAddr, codeMapAddr, count};
        p->symbol = sympol;
        p->count = i;
        if (i < n - 1) {
            p->next = new Stack();
        }
        p = p->next;
    }
    int coreId = 95;
    int numaId = 3;
    int sockId = 1;
    CpuTopology *cpu = new CpuTopology{coreId, numaId, sockId};
    unsigned long pa = 0xffff;
    unsigned long va = 0xfeee;
    unsigned long event = 1;
    PmuDataExt *ext = new PmuDataExt{pa, va, event};
    int64_t ts = 0xfffffff;
    pid_t pid = 10;
    int tid = 11;
    unsigned cpuId = 1;
    uint64_t period = 0xff12;
    SpeData data = {stack, "data1", ts, pid, tid, cpuId, cpu, "hello", period, ext};
    return data;
}

TEST(Serialize, SpeData)
{
    int n = 10;
    auto data = CreateSpeData(n);
    oeaware::OutStream out;
    out << data;
    oeaware::InStream in(out.Str());
    SpeData data1;
    in >> data1;
    auto tmpStack = data1.stack;
    auto stack = data.stack;
    for (int i = 0; i < n; ++i) {
        EXPECT_EQ(tmpStack->count, stack->count);
        EXPECT_EQ(tmpStack->symbol->addr, stack->symbol->addr);
        EXPECT_EQ(0, strcmp(tmpStack->symbol->module, stack->symbol->module));
        EXPECT_EQ(0, strcmp(tmpStack->symbol->fileName, stack->symbol->fileName));
        EXPECT_EQ(0, strcmp(tmpStack->symbol->symbolName, stack->symbol->symbolName));
        EXPECT_EQ(tmpStack->symbol->lineNum, stack->symbol->lineNum);
        EXPECT_EQ(tmpStack->symbol->offset, stack->symbol->offset);
        EXPECT_EQ(tmpStack->symbol->codeMapAddr, stack->symbol->codeMapAddr);
        EXPECT_EQ(tmpStack->symbol->codeMapEndAddr, stack->symbol->codeMapEndAddr);
        EXPECT_EQ(tmpStack->symbol->count, stack->symbol->count);
        tmpStack = tmpStack->next;
        stack = stack->next;
    }
    EXPECT_EQ(0, strcmp(data1.evt, data.evt));
    EXPECT_EQ(data1.ts, data.ts);
    EXPECT_EQ(data1.pid, data.pid);
    EXPECT_EQ(data1.tid, data.tid);
    EXPECT_EQ(data1.cpu, data.cpu);
    EXPECT_EQ(data1.cpuTopo->coreId, data.cpuTopo->coreId);
    EXPECT_EQ(data1.cpuTopo->socketId, data.cpuTopo->socketId);
    EXPECT_EQ(data1.cpuTopo->numaId, data.cpuTopo->numaId);
    EXPECT_EQ(0, strcmp(data1.comm, data.comm));
    EXPECT_EQ(data1.period, data.period);
    EXPECT_EQ(data1.ext->pa, data.ext->pa);
    EXPECT_EQ(data1.ext->va, data.ext->va);
    EXPECT_EQ(data1.ext->event, data.ext->event);
}
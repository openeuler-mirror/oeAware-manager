/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/

#include <securec.h>
#include <iostream>
#include "pmu_spe_data.h"
#include "symbol.h"
#include "utils.h"

oeaware::Register<PmuSpeData> PmuSpeData::pmuSpeReg("pmu_spe_collector");

void PmuSpeData::Serialize(oeaware::OutStream &out) const
{
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
}

void PmuSpeData::Deserialize(oeaware::InStream &in)
{
    in >> interval >> len;
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
            errno_t ret = strcpy_s(tmp->symbol->module, module.length() + 1, module.c_str());
            if (ret != EOK) {
                std::cout << "Deserialize failed, reason: strcpy_s failed" << std::endl;
                return;
            }
            tmp->symbol->symbolName = new char[symbolName.length() + 1];
            ret = strcpy_s(tmp->symbol->symbolName, symbolName.length() + 1, symbolName.c_str());
            if (ret != EOK) {
                std::cout << "Deserialize failed, reason: strcpy_s failed" << std::endl;
                return;
            }
            tmp->symbol->mangleName = new char[mangleName.length() + 1];
            ret = strcpy_s(tmp->symbol->mangleName, mangleName.length() + 1, mangleName.c_str());
            if (ret != EOK) {
                std::cout << "Deserialize failed, reason: strcpy_s failed" << std::endl;
                return;
            }
            tmp->symbol->fileName = new char[fileName.length() + 1];
            ret = strcpy_s(tmp->symbol->fileName, fileName.length() + 1, fileName.c_str());
            if (ret != EOK) {
                std::cout << "Deserialize failed, reason: strcpy_s failed" << std::endl;
                return;
            }
            tmp = tmp->next;
        }
        pmuData[i].cpuTopo = new CpuTopology();
        in >> pmuData[i].evt >> pmuData[i].ts >> pmuData[i].pid >> pmuData[i].tid >> pmuData[i].cpu
        >> pmuData[i].cpuTopo->coreId >> pmuData[i].cpuTopo->numaId >> pmuData[i].cpuTopo->socketId
        >> pmuData[i].comm >> pmuData[i].period >> pmuData[i].ext->pa >> pmuData[i].ext->va
        >> pmuData[i].ext->event;
    }
}
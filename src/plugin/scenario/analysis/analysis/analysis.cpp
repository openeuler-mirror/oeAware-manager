/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include <iostream>
#include <unordered_map>
#include <numaif.h>
#include "analysis.h"
#include <sstream>
#include <iomanip>
#include <cmath>

const int TUNE_PID_LOW_BOUND = 1000;
const uint64_t ACCESS_THRESHOLD = 200;
const float NUMA_SCORE_THRESHOLD = 0.95;
const int NET_RX_THRESHOLD = 100;
static bool IsValidPmu(const PmuData &data)
{
    return data.pid > TUNE_PID_LOW_BOUND && data.tid > TUNE_PID_LOW_BOUND;
}

static bool IsValidSpe(const PmuData &data)
{
    return IsValidPmu(data) && (data.ext->event & 0x200) != 0;  // 0x200 later change to enum value
}

int Analysis::GetPeriod()
{
    return 1000; // 1000 ms
}

void Analysis::Init()
{
    env.Init();
    InstanceInit();
    sysInfo.Init();  // initialized after env init
    loopCnt = 0;
}

void Analysis::UpdatePmu(const std::string &eventName, int dataLen, const PmuData *data)
{
    if (eventName == "pmu_spe_sampling") {
        UpdateSpe(dataLen, data);
        UpdateAccess();
    } else if (eventName == "pmu_netif_rx_counting") {
        UpdateNetRx(dataLen, data);
    }
}

void Analysis::InstanceInit()
{
    tuneInstances[NUMA_TUNE].name = "tune_numa_mem_access";
    tuneInstances[IRQ_TUNE].name = "tune_irq";
    tuneInstances[GAZELLE_TUNE].name = "gazelle";
    tuneInstances[SMC_TUNE].name = "smc_tune";
}

void Analysis::UpdateSpe(int dataLen, const PmuData *data)
{
    auto &procs = sysInfo.procs;
    for (int i = 0; i < dataLen; i++) {
        if (!IsValidSpe(data[i])) {
            continue;
        }
        procs[data[i].pid].pageVa.push_back((void *)data[i].ext->va);
        procs[data[i].pid].speBuff.push_back(&data[i]);
        procs[data[i].pid].threads[data[i].tid].cpu = data[i].cpu;
    }
}

void Analysis::UpdateAccess()
{
    const std::vector<int> &cpu2Node = Env::GetInstance().cpu2Node;
    auto &procs = sysInfo.procs;
    for (auto &proc_item : procs) {
        auto &proc = proc_item.second;
        int pid = proc_item.first;
        size_t buff_len = proc.pageVa.size();
        std::vector<int> node_id(buff_len);
        long ret = move_pages(pid, buff_len, proc.pageVa.data(), nullptr, node_id.data(), MPOL_MF_MOVE_ALL);
        if (ret == 0) {
            for (size_t i = 0; i < buff_len; ++i) {
                if (node_id[i] < 0) {
                    continue;
                }
                proc.threads[proc.speBuff[i]->tid].realtimeInfo.access[cpu2Node[proc.speBuff[i]->cpu]][node_id[i]]++;
            }
        }
        proc.pageVa.clear();
        proc.speBuff.clear();
    }
}

void Analysis::UpdateNetRx(int dataLen, const PmuData *data)
{
    auto &info = sysInfo.realtimeInfo.netInfo;
    for (int i = 0; i < dataLen; i++) {
        info.netRxTimes[env.cpu2Node[data[i].cpu]] += data[i].count;
        info.netRxSum += data[i].count;
    }
}

void Analysis::ShowSummary()
{
    std::cout << "============================ Analysis Summary ==============================" << std::endl;
    std::cout << "| ";
    const int nameWidth = 30;
    const int suggestWidth = 10;
    const int notesWidth = 30;
    std::cout << std::left << std::setw(nameWidth) << "Tune Instance" << " | ";
    std::cout << std::left << std::setw(suggestWidth) << "Suggest" << " | ";
    std::cout << std::left << std::setw(notesWidth) << "Note" << " |" << std::endl;

    for (auto &item : tuneInstances) {
        auto &ins = item.second;
        if (ins.name == "unknown") {
            continue;
        }
        std::cout << "| ";
        std::cout << std::left << std::setw(nameWidth) << ins.name << " | ";
        std::cout << std::left << std::setw(suggestWidth) << (ins.suggest ? "Yes" : "No") << " | ";
        std::cout << std::left << std::setw(notesWidth) << ins.notes << " |" << std::endl;
    }
    ShowNetInfoSummary();
    std::cout << "Note : analysis plugin period is " << GetPeriod() << " ms, loop " << loopCnt << " times" << std::endl;
}

void Analysis::NumaTuneSuggest(const TaskInfo &taskInfo, bool isSummary)
{
    tuneInstances[NUMA_TUNE].suggest = false;
    if (env.numaNum <= 1) {
        tuneInstances[NUMA_TUNE].notes = "No NUMA";
        return;
    }

    if (isSummary && taskInfo.loopCnt == 0) {
        tuneInstances[NUMA_TUNE].notes = "Loop count error";
        return;
    }
    uint64_t accessSum = isSummary ? ceil(taskInfo.accessSum * 1.0 / taskInfo.loopCnt) : taskInfo.accessSum;
    if (accessSum < ACCESS_THRESHOLD) {
        tuneInstances[NUMA_TUNE].notes = "No access";
        return;
    }
    if (taskInfo.numaScore > NUMA_SCORE_THRESHOLD) {
        tuneInstances[NUMA_TUNE].notes = "Most local access";
        return;
    }
    tuneInstances[NUMA_TUNE].suggest = true;
    std::ostringstream tmp;
    // 100 is used for percentage conversion, 2 is used for precision
    tmp << std::fixed << std::setprecision(2) << ((1 - taskInfo.numaScore) * 100);
    tuneInstances[NUMA_TUNE].notes = "Gap : " + tmp.str() + "%";
}

static bool IsFrequentLocalNetAccess(uint64_t times)
{
    return times > NET_RX_THRESHOLD;
}

void Analysis::NetTuneSuggest(const TaskInfo &taskInfo, bool isSummary)
{
    tuneInstances[IRQ_TUNE].suggest = false;
    tuneInstances[GAZELLE_TUNE].suggest = false;
    tuneInstances[SMC_TUNE].suggest = false;
    if (isSummary && taskInfo.loopCnt == 0) {
        tuneInstances[IRQ_TUNE].notes = "Loop count error";
        tuneInstances[GAZELLE_TUNE].notes = "Loop count error";
        tuneInstances[SMC_TUNE].notes = "Loop count error";
        return;
    }
    const auto &netInfo = taskInfo.netInfo;
    uint64_t times = isSummary ? ceil(netInfo.netRxSum * 1.0 / loopCnt) : netInfo.netRxSum;
    if (!IsFrequentLocalNetAccess(times)) {
        tuneInstances[IRQ_TUNE].notes = "No local Newtwork access";
        tuneInstances[GAZELLE_TUNE].notes = "No local Newtwork access";
        tuneInstances[SMC_TUNE].notes = "No local Newtwork access";
        return;
    }
    tuneInstances[IRQ_TUNE].suggest = true;
    tuneInstances[GAZELLE_TUNE].suggest = true;
    tuneInstances[SMC_TUNE].suggest = true;
    tuneInstances[IRQ_TUNE].notes = "Refer to network information";
    tuneInstances[GAZELLE_TUNE].notes = "Refer to network information";
    tuneInstances[SMC_TUNE].notes = "Refer to network information";
}

void Analysis::Summary()
{
    sysInfo.TraceInfoSummary();
    NumaTuneSuggest(sysInfo.summaryInfo, true);
    NetTuneSuggest(sysInfo.summaryInfo, true);
}

void Analysis::ShowNetInfoSummary()
{
    std::cout << "============================ Network Summary ==============================" << std::endl;
    // Display network traffic in the future
    std::cout << " Local network communication distribution " << std::endl;
    const auto &netInfo = sysInfo.summaryInfo.netInfo;
    std::cout << "  ";
    for (size_t n = 0; n < netInfo.netRxTimes.size(); n++) {
        std::cout << "Node" << std::to_string(n) << "   ";
    }
    std::cout << std::endl;
    for (size_t n = 0; n < netInfo.netRxTimes.size(); n++) {
        std::ostringstream tmp;
        // 2 is used for precision, 6 is used for width
        tmp << std::fixed << std::setprecision(2) << std::setw(6) << std::setfill(' ') \
            << (netInfo.netRxTimes[n] * 1.0 / netInfo.netRxSum * 100); // 100 is used for percentage conversion
        std::cout << tmp.str() << "% ";
    }
    std::cout << std::endl;
}

void Analysis::Analyze()
{
    sysInfo.SummaryProcs();
    sysInfo.CalculateNumaScore();
    sysInfo.SetLoopCnt(loopCnt);
    sysInfo.AppendTraceInfo();
    sysInfo.ClearRealtimeInfo();
    loopCnt++;
}

void Analysis::Exit()
{
    Summary();
    ShowSummary();
    sysInfo.Reset();
}

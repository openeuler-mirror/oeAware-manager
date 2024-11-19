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
#include "pmu_plugin.h"

const int TUNE_PID_LOW_BOUND = 1000;
const uint64_t ACCESS_THRESHOLD = 200;
const float NUMA_SCORE_THRESHOLD = 0.95;
const int NET_RX_THRESHOLD = 100;
const int NET_RX_RECEIVE_SAMPLE_PERIOD = 10;
const int NET_REMOTE_RX_THRESHOLD = 100;
const float STEAL_TASK_CYCLES_THRESHOLD = 0.8;
const int MS_PER_SEC = 1000;
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
    return 1 * MS_PER_SEC;
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
    } else if (eventName == "pmu_napi_gro_rec_entry") {
        UpdateNapiGroRec(dataLen, data);
    } else if (eventName == "pmu_skb_copy_datagram_iovec") {
        UpdateSkbCopyDataIovec(dataLen, data);
    } else if (eventName == "pmu_cycles_sampling") {
        UpdateCyclesSample(dataLen, data);
    }
}

void Analysis::InstanceInit()
{
    tuneInstances[NUMA_TUNE].name = "tune_numa_mem_access";
    tuneInstances[IRQ_TUNE].name = "tune_irq";
    tuneInstances[GAZELLE_TUNE].name = "gazelle";
    tuneInstances[SMC_TUNE].name = "smc_tune";
    tuneInstances[STEALTASK_TUNE].name = "stealtask_tune";
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

void Analysis::UpdateCyclesSample(int dataLen, const PmuData *data)
{
    auto &procs = sysInfo.procs;
    for (int i = 0; i < dataLen; i++) {
        if (!IsValidPmu(data[i])) {
            continue;
        }
        procs[data[i].pid].threads[data[i].tid].realtimeInfo.cycles += data[i].period;
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

void Analysis::UpdateNapiGroRec(int dataLen, const PmuData *data)
{
    RecNetQueue unit;
    NapiGroRecEntryData tmpData;
    for (int i = 0; i < dataLen; i++) {
        if (NapiGroRecEntryResolve(data[i].rawData->data, &tmpData)) {
            continue;
        }
        unit.core = data[i].cpu;
        unit.dev = std::string(tmpData.deviceName);
        unit.queueMapping = tmpData.queueMapping - 1;
        unit.ts = data[i].ts;
        unit.skbaddr = tmpData.skbaddr;
        recNetQueue.emplace_back(unit);
    }
}

void Analysis::UpdateSkbCopyDataIovec(int dataLen, const PmuData *data)
{
    RecNetThreads unit;
    SkbCopyDatagramIovecData tmpData;
    for (int i = 0; i < dataLen; i++) {
        if (SkbCopyDatagramIovecResolve(data[i].rawData->data, &tmpData)) {
            continue;
        }
        unit.core = data[i].cpu;
        unit.ts = data[i].ts;
        unit.pid = data[i].pid;
        unit.tid = data[i].tid;
        unit.skbaddr = tmpData.skbaddr;
        recNetThreads.emplace_back(unit);
    }
}
void Analysis::UpdateRemoteNetInfo(const RecNetQueue &queData, const RecNetThreads &threadData)
{
    const std::vector<int> &cpu2Node = env.cpu2Node;
    int pid = threadData.pid;
    int tid = threadData.tid;
    uint8_t threadsNode = cpu2Node[threadData.core];
    uint8_t irqNode = cpu2Node[queData.core];
    auto &procs = sysInfo.procs;
    procs[pid].threads[tid].realtimeInfo.netInfo.rxTimes[queData.dev][queData.queueMapping][threadsNode][irqNode]++;
}


void Analysis::UpdateRecNetQueue()
{
    size_t id = 0, idNext = 0, matchTimes = 0;
    for (const auto &queueData : recNetQueue) {
        for (size_t j = id; j < recNetThreads.size(); ++j) {
            if (recNetThreads[j].ts >= queueData.ts) {
                idNext = j;
                break;
            }
        }
        id = idNext;
        // 200 : sliding window length
        for (size_t j = id; j < recNetThreads.size() && j < id + 200; ++j) {
            if (queueData.skbaddr == recNetThreads[j].skbaddr) {
                UpdateRemoteNetInfo(queueData, recNetThreads[j]);
                ++matchTimes;
                break;
            }
        }
    }
    recNetQueue.clear();
    recNetThreads.clear();
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

static bool IsFrequentRemoteRecNetAccess(uint64_t times)
{
    return times > NET_REMOTE_RX_THRESHOLD;
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
    uint64_t remoteRxtimes = isSummary ? \
        ceil(netInfo.remoteRxSum * NET_RX_RECEIVE_SAMPLE_PERIOD * 1.0 / loopCnt) : netInfo.remoteRxSum;
    if (IsFrequentLocalNetAccess(times) || IsFrequentRemoteRecNetAccess(remoteRxtimes)) {
        tuneInstances[IRQ_TUNE].suggest = true;
        tuneInstances[GAZELLE_TUNE].suggest = true;
        tuneInstances[SMC_TUNE].suggest = true;
        tuneInstances[IRQ_TUNE].notes = "Refer to network information";
        tuneInstances[GAZELLE_TUNE].notes = "Refer to network information";
        tuneInstances[SMC_TUNE].notes = "Refer to network information";
    } else {
        tuneInstances[IRQ_TUNE].notes = "No network access";
        tuneInstances[GAZELLE_TUNE].notes = "No network access";
        tuneInstances[SMC_TUNE].notes = "No network access";
    }
}

void Analysis::StealTaskTuneSuggest(const TaskInfo &taskInfo, bool isSummary)
{
    tuneInstances[STEALTASK_TUNE].suggest = false;
    if (isSummary && taskInfo.loopCnt == 0) {
        tuneInstances[STEALTASK_TUNE].notes = "Loop count error";
        return;
    }
    uint64_t sysCyclesPerSecond = isSummary ? ceil(taskInfo.cycles * 1.0 / taskInfo.loopCnt) : taskInfo.cycles;
    sysCyclesPerSecond = sysCyclesPerSecond / GetPeriod() * MS_PER_SEC; // later use real time
    float cpuRatio = sysCyclesPerSecond * 1.0 / env.sysMaxCycles;
    if (cpuRatio > STEAL_TASK_CYCLES_THRESHOLD) {
        tuneInstances[STEALTASK_TUNE].suggest = true;
    }
    std::ostringstream tmp;
    // 2 is used for precision, 6 is used for width, 100.0 is used for percentage conversion
    tmp << std::fixed << std::setprecision(2) << std::setw(6) << std::setfill(' ') << cpuRatio * 100.0 << "%";
    tuneInstances[STEALTASK_TUNE].notes = "CpuRatio(average) : " + tmp.str();
}

void Analysis::Summary()
{
    sysInfo.TraceInfoSummary();
    NumaTuneSuggest(sysInfo.summaryInfo, true);
    NetTuneSuggest(sysInfo.summaryInfo, true);
    StealTaskTuneSuggest(sysInfo.summaryInfo, true);
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
    std::cout << " Remote network communication distribution(receive) " << std::endl;
    std::vector<std::vector<uint64_t>> remoteRxTimes(env.numaNum, std::vector<uint64_t>(env.numaNum, 0));
    netInfo.Node2NodeRxTimes(remoteRxTimes);
    std::cout << " matrix representation of network thread nodes to irq nodes" << std::endl;
    std::cout << "         ";
    for (size_t n = 0; n < remoteRxTimes.size(); n++) {
        std::cout << "Node" << std::to_string(n) << "   ";
    }
    std::cout << std::endl;
    for (size_t n = 0; n < remoteRxTimes.size(); n++) {
        std::cout << " Node" << std::to_string(n) << " ";
        for (size_t m = 0; m < remoteRxTimes[n].size(); m++) {
            std::ostringstream tmp;
            // 2 is used for precision, 6 is used for width
            tmp << std::fixed << std::setprecision(2) << std::setw(6) << std::setfill(' ') \
                << remoteRxTimes[n][m] * 100.0 / netInfo.remoteRxSum;  // 100.0 is used for percentage conversion
            std::cout << tmp.str() << "% ";
        }
        std::cout << std::endl;
    }
    std::cout << "remote network receive " << \
        netInfo.remoteRxSum * NET_RX_RECEIVE_SAMPLE_PERIOD << " packets. " << std::endl;
}

void Analysis::Analyze()
{
    UpdateRecNetQueue();
    sysInfo.SummaryProcs();
    sysInfo.CalculateNumaScore();
    sysInfo.SummaryProcsNetInfo();
    sysInfo.SetLoopCnt(loopCnt);
    sysInfo.AppendTraceInfo();
    sysInfo.ClearRealtimeInfo();
    loopCnt++;
}

void Analysis::Exit()
{
    sysInfo.Reset();
}

void Analysis::BuildNetInfoSummary()
{
	std::ostringstream output;
    output << "============================ Network Summary ==============================";
	analysisData.push_back(output.str());
	output.str("");
    // Display network traffic in the future
    output << " Local network communication distribution ";
	analysisData.push_back(output.str());
	output.str("");
    const auto &netInfo = sysInfo.summaryInfo.netInfo;
    output << "  ";
    for (size_t n = 0; n < netInfo.netRxTimes.size(); n++) {
        output << "Node" << std::to_string(n) << "   ";
    }
	analysisData.push_back(output.str());
	output.str("");
    for (size_t n = 0; n < netInfo.netRxTimes.size(); n++) {
        // 2 is used for precision, 6 is used for width
        output << std::fixed << std::setprecision(2) << std::setw(6) << std::setfill(' ') \
            << (netInfo.netRxTimes[n] * 1.0 / netInfo.netRxSum * 100); // 100 is used for percentage conversion
        output << "% ";
    }
	analysisData.push_back(output.str());
	output.str("");
    output << " Remote network communication distribution(receive) ";
	analysisData.push_back(output.str());
	output.str("");
    std::vector<std::vector<uint64_t>> remoteRxTimes(env.numaNum, std::vector<uint64_t>(env.numaNum, 0));
    netInfo.Node2NodeRxTimes(remoteRxTimes);
    output<< " matrix representation of network thread nodes to irq nodes";
	analysisData.push_back(output.str());
	output.str("");	
    output << "         ";
    for (size_t n = 0; n < remoteRxTimes.size(); n++) {
        output << "Node" << std::to_string(n) << "   ";
    }
    analysisData.push_back(output.str());
	output.str("");
    for (size_t n = 0; n < remoteRxTimes.size(); n++) {
        output << " Node" << std::to_string(n) << " ";
        for (size_t m = 0; m < remoteRxTimes[n].size(); m++) {
            // 2 is used for precision, 6 is used for width
            output << std::fixed << std::setprecision(2) << std::setw(6) << std::setfill(' ') \
                << remoteRxTimes[n][m] * 100.0 / netInfo.remoteRxSum;  // 100.0 is used for percentage conversion
            output  << "% ";
        }
		analysisData.push_back(output.str());
		output.str("");
    }
    output << "remote network receive " << \
        netInfo.remoteRxSum * NET_RX_RECEIVE_SAMPLE_PERIOD << " packets. ";
	analysisData.push_back(output.str());
}

void Analysis::BuildSummary()
{
	std::ostringstream output;
	analysisData.clear();
	output << "============================ Analysis Summary ==============================";
	analysisData.push_back(output.str());
	output.str("");
	
	output << "| ";
	const int nameWidth = 30;
	const int suggestWidth = 10;
	const int notesWidth = 30;
	output << std::left << std::setw(nameWidth) << "Tune Instance" << " | ";
	output << std::left << std::setw(suggestWidth) << "Suggest" << " | ";
	output << std::left << std::setw(notesWidth) << "Note" << " |" ;
	analysisData.push_back(output.str());
	output.str("");

	for (auto &item : tuneInstances) {
		auto &ins = item.second;
		if (ins.name == "unknown") {
			continue;
		}
		output << "| ";
		output << std::left << std::setw(nameWidth) << ins.name << " | ";
		output << std::left << std::setw(suggestWidth) << (ins.suggest ? "Yes" : "No") << " | ";
		output << std::left << std::setw(notesWidth) << ins.notes << " |";
		analysisData.push_back(output.str());
		output.str("");
	}
	BuildNetInfoSummary();
	output << "Note : analysis plugin period is " << GetPeriod() << " ms, loop " << loopCnt << " times";
	analysisData.push_back(output.str());
}

std::vector<std::string> Analysis::GetAnalysisData()
{
	Summary();
    BuildSummary();
	return std::move(analysisData);
}


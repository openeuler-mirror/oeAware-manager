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
#include "common.h"
#include <iostream>

void TraceInfoSummary(const std::vector<TaskInfo> &infos, TaskInfo &summary)
{
    for (auto &info : infos) {
        for (size_t i = 0; i < info.access.size(); i++) {
            for (size_t j = 0; j < info.access[i].size(); j++) {
                summary.access[i][j] += info.access[i][j];
            }
        }
        for (size_t i = 0; i < info.netInfo.netRxTimes.size(); i++) {
            summary.netInfo.netRxTimes[i] += info.netInfo.netRxTimes[i];
        }
        summary.cycles += info.cycles;
        summary.netInfo.netRxSum += info.netInfo.netRxSum;
        summary.netInfo.AddRxTimes(info.netInfo.rxTimes);
    }
    summary.loopCnt = infos.size();
    summary.CalculateNumaScore();
    summary.netInfo.SumRemoteRxTimes();
}

TaskInfo::TaskInfo()
{
    Init();
}

void TaskInfo::Init()
{
    loopCnt = 0;
    int numaNum = Env::GetInstance().numaNum;
    access.resize(numaNum);
    for (int i = 0; i < numaNum; i++) {
        access[i].resize(numaNum);
    }
    netInfo.Init();
}

void TaskInfo::CalculateNumaScore()
{
    Env &env = Env::GetInstance();
    accessCost = 0;
    accessSum = 0;
    for (int i = 0; i < env.numaNum; i++) {
        for (int j = 0; j < env.numaNum; j++) {
            accessCost += access[i][j] * env.distance[i][j];
            accessSum += access[i][j];
        }
    }
    numaScore = accessSum == 0 ? 0.0 \
        : (accessSum * env.maxDistance - accessCost) * 1.0 / (accessSum * env.diffDistance);
}

void TaskInfo::ClearData()
{
    for (auto &tmp : access) {
        for (auto &i : tmp) { i = 0; }
    }
    numaScore = 0.0;
    cycles = 0;
    accessSum = 0;
    accessCost = 0;
    netInfo.ClearData();
}

void Proc::SummaryThreads()
{
    for (auto &t : threads) {
        auto &thread = t.second;
        for (size_t i = 0; i < thread.realtimeInfo.access.size(); i++) {
            for (size_t j = 0; j < thread.realtimeInfo.access[i].size(); j++) {
                realtimeInfo.access[i][j] += thread.realtimeInfo.access[i][j];
            }
        }
        realtimeInfo.cycles += thread.realtimeInfo.cycles;
    }
}

void Proc::CalculateNumaScore()
{
    for (auto &thread : threads) {
        thread.second.realtimeInfo.CalculateNumaScore();
    }
    realtimeInfo.CalculateNumaScore();
}

void Proc::ClearRealtimeInfo()
{
    for (auto &t : threads) {
        auto &thread = t.second;
        thread.realtimeInfo.ClearData();
    }
    realtimeInfo.ClearData();
}

void SystemInfo::Init()
{
    realtimeInfo.Init();
    summaryInfo.Init();
}

void SystemInfo::SummaryProcs()
{
    for (auto &proc_item : procs) {
        auto &proc = proc_item.second;
        proc.SummaryThreads();
        for (size_t i = 0; i < proc.realtimeInfo.access.size(); i++) {
            for (size_t j = 0; j < proc.realtimeInfo.access[i].size(); j++) {
                realtimeInfo.access[i][j] += proc.realtimeInfo.access[i][j];
            }
        }
        realtimeInfo.cycles += proc.realtimeInfo.cycles;
    }
}

void SystemInfo::CalculateNumaScore()
{
    for (auto &proc_item : procs) {
        auto &proc = proc_item.second;
        proc.CalculateNumaScore();
    }
    realtimeInfo.CalculateNumaScore();
}

void SystemInfo::SummaryProcsNetInfo()
{
    for (auto &proc_item : procs) {
        auto &proc = proc_item.second;
        for (auto &thread_item : proc.threads) {
            proc.realtimeInfo.netInfo.AddRxTimes(thread_item.second.realtimeInfo.netInfo.rxTimes);
            thread_item.second.realtimeInfo.netInfo.SumRemoteRxTimes();
        }
        proc.realtimeInfo.netInfo.SumRemoteRxTimes();
        realtimeInfo.netInfo.AddRxTimes(proc.realtimeInfo.netInfo.rxTimes);
    }
    realtimeInfo.netInfo.SumRemoteRxTimes();
}

void SystemInfo::SetLoopCnt(uint64_t loopCnt)
{
    for (auto &proc_item : procs) {
        auto &proc = proc_item.second;
        for (auto &thread_item : proc.threads) {
            auto &thread = thread_item.second;
            thread.realtimeInfo.loopCnt = loopCnt;
        }
        proc.realtimeInfo.loopCnt = loopCnt;
    }
    realtimeInfo.loopCnt = loopCnt;
}

void SystemInfo::ClearRealtimeInfo()
{
    for (auto &proc_item : procs) {
        proc_item.second.ClearRealtimeInfo();
    }

    realtimeInfo.ClearData();
}

void SystemInfo::AppendTraceInfo()
{
    for (auto &proc_item : procs) {
        auto &proc = proc_item.second;
        for (auto &thread_item : proc.threads) {
            auto thread = thread_item.second;
            thread.traceInfo.emplace_back(thread.realtimeInfo);
        }
        proc.traceInfo.emplace_back(proc.realtimeInfo);
    }
    traceInfo.emplace_back(realtimeInfo);
}

void SystemInfo::TraceInfoSummary()
{
    for (auto &proc_item : procs) {
        auto &proc = proc_item.second;
        for (auto &thread_item : proc.threads) {
            auto thread = thread_item.second;
            ::TraceInfoSummary(thread.traceInfo, thread.summaryInfo);
        }
        ::TraceInfoSummary(proc.traceInfo, proc.summaryInfo);
    }
    ::TraceInfoSummary(traceInfo, summaryInfo);
}

void SystemInfo::Reset()
{
    procs.clear();
    realtimeInfo.ClearData();
    summaryInfo.ClearData();
    traceInfo.clear();
}

NetworkInfo::NetworkInfo()
{
    Init();
}

void NetworkInfo::Init()
{
    int numaNum = Env::GetInstance().numaNum;
    netRxTimes.resize(numaNum);
}

void NetworkInfo::AddRxTimes(const std::unordered_map<std::string,
    std::unordered_map<int, std::unordered_map<uint8_t, std::unordered_map<uint8_t, uint64_t>>>> &value)
{
    for (const auto &intf : value) {
        for (const auto &queue : intf.second) {
            for (const auto &thrNode : queue.second) {
                for (const auto &irqNode : thrNode.second) {
                    rxTimes[intf.first][queue.first][thrNode.first][irqNode.first] += irqNode.second;
                }
            }
        }
    }
}

void NetworkInfo::Node2NodeRxTimes(std::vector<std::vector<uint64_t>> &value) const
{
    for (auto &intf : rxTimes) {
        for (auto &queue : intf.second) {
            for (auto &thrNode : queue.second) {
                for (auto &irqNode : thrNode.second) {
                    value[thrNode.first][irqNode.first] += irqNode.second;
                }
            }
        }
    }
}

void NetworkInfo::SumRemoteRxTimes()
{
    remoteRxSum = 0;
    for (const auto &intf : rxTimes) {
        for (const auto &queue : intf.second) {
            for (const auto &thrNode : queue.second) {
                for (const auto &irqNode : thrNode.second) {
                    remoteRxSum += irqNode.second;
                }
            }
        }
    }
}

void NetworkInfo::ClearData()
{
    netRxSum = 0;
    remoteRxSum = 0;
    for (auto &tmp : netRxTimes) {
        tmp = 0;
    }
    rxTimes.clear();
}

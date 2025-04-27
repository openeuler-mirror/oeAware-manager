/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include "net_hirq_analysis.h"
#include <algorithm>
#include <string>
#include <iomanip>
#include "analysis_utils.h"
#include "oeaware/data/pmu_plugin.h"
#include "oeaware/data/pmu_sampling_data.h"

namespace oeaware {
const int MS_PER_SEC = 1000;                // 1000 : ms per sec
const int SHOW_DEV_MAX_NUM = 3;             // 3 : show dev max num
const int HIRQ_TUNE_PEAK_THRESHOLD = 1000;  // 1000 queue data per second

static std::string FloatToString(float value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

NetHirqAnalysis::NetHirqAnalysis()
{
    name = OE_NET_HIRQ_ANALYSIS;
    version = "1.0.0";
    period = 1000;       // 1000 ms
    priority = 1;        // 1: scenario instance
    type = SCENARIO;
    subscribeTopics.emplace_back(oeaware::Topic{ OE_PMU_SAMPLING_COLLECTOR, "net:napi_gro_receive_entry", "" });
    for (const auto &it : topicStr) {
        oeaware::Topic topic;
        topic.instanceName = this->name;
        topic.topicName = it;
        topic.params = "";
        supportTopics.emplace_back(topic);
        topicCtl[it].topicName = it;
    }
}

Result NetHirqAnalysis::OpenTopic(const oeaware::Topic &topic)
{
    if (topicCtl.count(topic.topicName) == 0) {
        return oeaware::Result(FAILED, "topic not register");
    }
    if (topicCtl[topic.topicName].isOpen) {
        return oeaware::Result(FAILED, "topic already opened");
    }
    auto paramsMap = GetKeyValueFromString(topic.params);
	if (paramsMap.count("t")) {
		topicCtl[topic.topicName].analysisTime = atoi(paramsMap["t"].data());
	}

    topicCtl[topic.topicName].beginTime = std::chrono::high_resolution_clock::now();
    topicCtl[topic.topicName].isOpen = true;
    topicCtl[topic.topicName].openParams = topic.params;
    return oeaware::Result(OK);
}

void NetHirqAnalysis::CloseTopic(const oeaware::Topic &topic)
{
    if (topicCtl.count(topic.topicName) == 0) {
        return;
    }
    if (!topicCtl[topic.topicName].isOpen) {
        return;
    }
    topicCtl[topic.topicName].isOpen = false;
    topicCtl[topic.topicName].analysisTime = 0;
    topicCtl[topic.topicName].openParams = "";
    topicCtl[topic.topicName].hasPublished = false;
}

AnalysisRst NetHirqAnalysis::GetAnalysisResult(const std::string &dev, const std::vector<NetRx> &netRxVec)
{
    AnalysisRst ret;
    ret.dev = dev;
    float peak = 0;
    uint64_t sum = 0;
    uint64_t interval = 0;
    for (auto &netRx : netRxVec) {
        if (peak < netRx.rxAvg) {
            peak = netRx.rxAvg;
        }
        sum += netRx.rxSum;
        interval += netRx.interval;
    }
    float avg = static_cast<float>(sum) / interval;
    ret.shouldTune = peak > HIRQ_TUNE_PEAK_THRESHOLD ? true : false;
    ret.peak = peak;
    ret.average = avg;
    return ret;
}

void NetHirqAnalysis::GenPublishData()
{
    std::vector<AnalysisRst> analysisRst;
    int shouldTuneNum = 0;
    for (const auto &item : result) {
        if (item.second.shouldTune) {
            shouldTuneNum++;
        }
        analysisRst.emplace_back(item.second);
    }
    std::sort(analysisRst.begin(), analysisRst.end(), [](const AnalysisRst &a, const AnalysisRst &b) {
        return a.peak > b.peak;
        });

    std::vector<std::vector<std::string>> metrics;
    std::vector<int> type;
     // system may have multiple network interface, only show showDevNum result
    int showDevNum = analysisRst.size() > SHOW_DEV_MAX_NUM ? SHOW_DEV_MAX_NUM : analysisRst.size();
    std::string conclusion;
    if (showDevNum <= 0) {
        conclusion += "net analysis: no high hirq net device needs tuning";
    } else {
        conclusion = conclusion + "net analysis, show top " + std::to_string(showDevNum) + " high peak devices:\n";
    }

    for (int i = 0; i < showDevNum; ++i) {
        const std::string peakDesc = "peak:" + FloatToString(analysisRst[i].peak) + " (hirq/s)";
        const std::string avgDesc = "avg:" + FloatToString(analysisRst[i].average) + " (hirq/s)";
        const std::string note = analysisRst[i].shouldTune ? "high hirq freq" : "low hirq freq";
        type.emplace_back(DATA_TYPE_NETWORK);
        conclusion += "   " + analysisRst[i].dev + " " + peakDesc + ", " + avgDesc + "\n";
        metrics.emplace_back(std::vector<std::string>{analysisRst[i].dev, peakDesc, note});
    }

    std::vector<std::string> suggestionItem;
    if (shouldTuneNum > 0) {
        suggestionItem.emplace_back("use net hard irq tune");
        suggestionItem.emplace_back("oeawarectl -e net_hard_irq_tune");
        suggestionItem.emplace_back("affinity network threads and interrupts");
    }
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
}

void NetHirqAnalysis::UpdateData(const DataList &dataList)
{
    std::string topicName = dataList.topic.topicName;
    if (topicName == "net:napi_gro_receive_entry") {
        PmuSamplingData *dataTmp = static_cast<PmuSamplingData *>(dataList.data[0]);
        NapiGroRecEntryData tmpData;
        for (int i = 0; i < dataTmp->len; i++) {
            if (NapiGroRecEntryResolve(dataTmp->pmuData[i].rawData->data, &tmpData)) {
                continue;
            }
            const std::string dev = std::string(tmpData.deviceName);
            netRxSum[dev].rxSum += 10; // 10 : NET_RECEIVE_TRACE_SAMPLE_PERIOD
        }
        for (auto &dev : netRxSum) {
            dev.second.interval += dataTmp->interval;
        }
    }
}

Result NetHirqAnalysis::Enable(const std::string &param)
{
    (void)param;
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }
    return oeaware::Result(OK);
}

void NetHirqAnalysis::Disable()
{
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
}
void NetHirqAnalysis::Run()
{
    for (auto &devIt : netRxSum) {
        auto &dev = devIt.second;
        dev.rxAvg = dev.rxSum * MS_PER_SEC / dev.interval;
        netRxSumTrace[devIt.first].emplace_back(dev);
    }
    auto now = std::chrono::system_clock::now();
    for (auto &it : topicCtl) {
        auto &info = it.second;
        int curTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.beginTime).count();
        if (curTimeMs / MS_PER_SEC < info.analysisTime) {
            continue;
        }
        // only publish once
        if (info.topicName == OE_NET_HIRQ_ANALYSIS && !info.hasPublished) {
            for (auto &devIt : netRxSumTrace) {
                result[devIt.first] = GetAnalysisResult(devIt.first, devIt.second);
            }
            GenPublishData();
            DataList dataList;
            SetDataListTopic(&dataList, name, info.topicName, info.openParams);
            dataList.len = 1;
            dataList.data = new void *[dataList.len];
            dataList.data[0] = &analysisResultItem;
            Publish(dataList);
            info.hasPublished = true;
        }
    }
    netRxSum.clear();
}
}
#include "hardirq_tune.h"
#include "oeaware/data/env_data.h"
#include "oeaware/data/pmu_plugin.h"
#include "oeaware/data/pmu_sampling_data.h"

using namespace oeaware;

NetHardIrq::NetHardIrq()
{
    name = OE_NETHARDIRQ_TUNE;
    version = "1.0.0";
    period = 1000;       // 1000 ms
    priority = 2;        // 2: tune instance
    type = TUNE;

    subscribeTopics.emplace_back(oeaware::Topic{ OE_ENV_INFO, "static", "" });
    subscribeTopics.emplace_back(oeaware::Topic{ OE_PMU_SAMPLING_COLLECTOR, "skb:skb_copy_datagram_iovec", "" });
    subscribeTopics.emplace_back(oeaware::Topic{ OE_PMU_SAMPLING_COLLECTOR, "net:napi_gro_receive_entry", "" });

    description += "[introduction] \n";
    description += "               Bind the interrupt corresponding to the network interface queue to NUMA, \n";
    description += "               which receives packets frequently in the network, to accelerate network processing\n";
    description += "[version] \n";
    description += "         " + version + "\n";
    description += "[instance name]\n";
    description += "                 " + name + "\n";
    description += "[instance period]\n";
    description += "                 " + std::to_string(period) + " ms\n";
    description += "[provide topics]\n";
    description += "                 none\n";
    description += "[running require arch]\n";
    description += "                      aarch64 only\n";
    description += "[running require environment]\n";
    description += "                 physical machine(not qemu), kernel version(4.19, 5.10, 6.6)\n";
	description += "[running require topics]\n";
	description += "                        1.env_info_collector::static\n";
	description += "                        2.pmu_sampling_collector::skb:skb_copy_datagram_iovec\n";
	description += "                        3.pmu_sampling_collector::net:napi_gro_receive_entry\n";
    description += "[usage] \n";
    description += "        example:You can use `oeawarectl -e net_hard_irq_tune` to enable\n";
}

oeaware::Result NetHardIrq::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void NetHardIrq::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void oeaware::NetHardIrq::UpdateSystemInfo(const DataList &dataList)
{
    std::string topicName = dataList.topic.topicName;
    if (topicName == "static" && !envInit) {
        EnvStaticInfo *dataTmp = static_cast<EnvStaticInfo *>(dataList.data[0]);
        numaNuma = dataTmp->numaNum;
        cpu2Numa.resize(dataTmp->cpuNumConfig);
        for (int i = 0; i < dataTmp->cpuNumConfig; i++) {
            cpu2Numa[i] = dataTmp->cpu2Node[i];
        }
        envInit = true;
    }
}

void oeaware::NetHardIrq::UpdateNetInfo(const DataList &dataList)
{
    std::string topicName = dataList.topic.topicName;
    PmuSamplingData *dataTmp = static_cast<PmuSamplingData *>(dataList.data[0]);
    if (topicName == "skb:skb_copy_datagram_iovec") {
        UpdateThreadData(dataTmp->pmuData, dataTmp->len);
        netDataInterval += dataTmp->interval;
    } else if (topicName == "net:napi_gro_receive_entry") {
        UpdateQueueData(dataTmp->pmuData, dataTmp->len);
    }
}

void NetHardIrq::UpdateQueueData(const PmuData *data, int dataLen)
{
    RecNetQueue unit;
    NapiGroRecEntryData tmpData;
    for (int i = 0; i < dataLen; i++) {
        if (NapiGroRecEntryResolve(data[i].rawData->data, &tmpData)) {
            continue;
        }
        unit.dev = std::string(tmpData.deviceName);
        unit.queueMapping = tmpData.queueMapping - 1;
        unit.ts = data[i].ts;
        unit.skbaddr = tmpData.skbaddr;
        queueData.emplace_back(unit);
    }
}

void NetHardIrq::UpdateThreadData(const PmuData *data, int dataLen)
{
    RecNetThreads unit;
    SkbCopyDatagramIovecData tmpData;
    for (int i = 0; i < dataLen; i++) {
        if (SkbCopyDatagramIovecResolve(data[i].rawData->data, &tmpData)) {
            continue;
        }
        unit.core = data[i].cpu;
        unit.ts = data[i].ts;
        unit.skbaddr = tmpData.skbaddr;
        threadData.emplace_back(unit);
    }
}

void NetHardIrq::UpdateThreadAndQueueInfo(const RecNetQueue &queData, const RecNetThreads &thrData)
{
    // to do
    (void)queData;
    (void)thrData;
}

void NetHardIrq::MatchThreadAndQueue()
{
    size_t id = 0;
    size_t idNext = 0;
    size_t matchTimes = 0;
    for (const auto &que : queueData) {
        for (size_t j = id; j < threadData.size(); ++j) {
            /* thread run after irq finish,
             * find the first thread which ts is greater than queue ts
             */
            if (threadData[j].ts >= que.ts) {
                idNext = j;
                break;
            }
        }
        id = idNext;
        /* some trace data is lost, use sliding window to avoid calc too much */
        for (size_t j = id; j < threadData.size() && j < id + slidingWinLen; ++j) {
            if (que.skbaddr == threadData[j].skbaddr) {
                UpdateThreadAndQueueInfo(que, threadData[j]);
                ++matchTimes;
                break;
            }
        }
    }
    threadData.clear();
    queueData.clear();
}

void NetHardIrq::UpdateData(const DataList &dataList)
{
    UpdateSystemInfo(dataList);
    UpdateNetInfo(dataList);
}

oeaware::Result NetHardIrq::Enable(const std::string &param)
{
    bool isActive;
    irqbalanceStatus = false;
    // irq tune confilct with irqbalance
    if (ServiceIsActive("irqbalance", isActive))
    {
        irqbalanceStatus = isActive;
    }
    if (irqbalanceStatus) {
        ServiceControl("irqbalance", "stop");
    }
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }
    return oeaware::Result(OK);
}

void NetHardIrq::Disable()
{
    if (irqbalanceStatus) {
        ServiceControl("irqbalance", "start");
    }
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
}

void NetHardIrq::Run()
{
    if (!envInit) {
        return;
    }
    MatchThreadAndQueue();
    netDataInterval = 0;
}

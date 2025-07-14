#include "hardirq_tune.h"
#include <sstream>
#include <iomanip>
#include <dirent.h>
#include "oeaware/data/env_data.h"
#include "oeaware/data/pmu_plugin.h"
#include "oeaware/data/pmu_sampling_data.h"
#include "oeaware/data/network_interface_data.h"

using namespace oeaware;

static const std::string HARDIRQ_CONFIG_PATH = oeaware::DEFAULT_PLUGIN_CONFIG_PATH + "/hardirq_tune.conf";

NetHardIrq::NetHardIrq()
{
    name = OE_NETHARDIRQ_TUNE;
    version = "1.0.0";
    period = 1000;       // 1000 ms
    priority = 2;        // 2: tune instance
    type = TUNE;
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
    supportTopics.emplace_back(Topic{ .instanceName = OE_NETHARDIRQ_TUNE,
        .topicName = OE_TOPIC_NET_HIRQ_TUNE_DEBUG_INFO, .params = "" });
}

oeaware::Result NetHardIrq::OpenTopic(const oeaware::Topic &topic)
{
    if (topic.instanceName != this->name) {
        return oeaware::Result(FAILED, "OpenTopic:" + topic.GetType() + " failed, instanceName is not match");
    }

    if (topic.params != "") {
        return oeaware::Result(FAILED, "OpenTopic:" + topic.GetType() + " failed, params is not empty");
    }

    if (topic.topicName != OE_TOPIC_NET_HIRQ_TUNE_DEBUG_INFO) {
        return oeaware::Result(FAILED, "OpenTopic:" + topic.GetType() + " failed, topicName is not match");
    }
    debugTopicOpen = true;
    return oeaware::Result(OK);
}

void NetHardIrq::CloseTopic(const oeaware::Topic &topic)
{
    if (debugTopicOpen && topic.topicName == OE_TOPIC_NET_HIRQ_TUNE_DEBUG_INFO) {
        debugTopicOpen = false;
    }
}
void NetHardIrq::Init()
{
    showVerbose = false;
    debugLog = "";
    runCnt = 0;
    InitIrqInfo();
}

void NetHardIrq::InitIrqInfo()
{
    const char *irqPath = "/proc/irq";
    DIR *dir = opendir(irqPath);

    if (dir == nullptr) {
        ERROR(logger, "NetHardIrq failed to open " << irqPath);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (!std::isdigit(entry->d_name[0])) {
            continue;
        }
        int irqNum;
        try {
            irqNum = std::stoi(entry->d_name);
        }
        catch (...) {
            WARN(logger, "Failed to parse IRQ number: " << entry->d_name);
            continue;
        }
        irqInfo[irqNum].originAffinity = IrqGetSmpAffinity(irqNum);
    }
    closedir(dir);
}

bool NetHardIrq::ResolveCmd(const std::string &param)
{
    if (param.empty()) {
        return true;
    }
    auto paramsMap = GetKeyValueFromString(param);
    for (auto &item : paramsMap) {
        if (!cmdHelp.count(item.first)) {
            ERROR(logger, "invalid param: " << item.first);
            return false;
        }
    }
    if (paramsMap.count("verbose")) {
        if (paramsMap["verbose"] == "on") {
            showVerbose = true;
        } else if (paramsMap["verbose"] == "off") {
            showVerbose = false;
        } else {
            ERROR(logger, "verbose invalid param: " << paramsMap["verbose"]);
            return false;
        }
    }
    if (paramsMap.count("netdata")) {
        if (paramsMap["netdata"] == "thread_recv_que") {
            highNoiseSample = false;
        } else if (paramsMap["netdata"] == "skb_copy") {
            highNoiseSample = true;
        } else {
            ERROR(logger, "netdata invalid param: " << paramsMap["netdata"]);
        }
    }
    return true;
}

std::string NetHardIrq::GetHelp()
{
    std::string output;
    output += "Usage : oeawarectl -e " + name + " [options] \n";
    for (const auto &item : cmdHelp) {
        output +=  item.second + " \n";
    }
    return output;
}

void oeaware::NetHardIrq::UpdateSystemInfo(const DataList &dataList)
{
    std::string topicName = dataList.topic.topicName;
    if (topicName == "static" && !envInit) {
        EnvStaticInfo *dataTmp = static_cast<EnvStaticInfo *>(dataList.data[0]);
        numaNum = dataTmp->numaNum;
        cpu2Numa.resize(dataTmp->cpuNumConfig);
        for (int i = 0; i < dataTmp->cpuNumConfig; i++) {
            cpu2Numa[i] = dataTmp->cpu2Node[i];
        }
        envInit = true;
    }
}

void oeaware::NetHardIrq::UpdatePmuSampleInfo(const DataList &dataList)
{
    std::string topicName = dataList.topic.topicName;
    PmuSamplingData *dataTmp = static_cast<PmuSamplingData *>(dataList.data[0]);
    if (topicName == "skb:skb_copy_datagram_iovec") {
        UpdateThreadData(dataTmp->pmuData, dataTmp->len);
        netDataInterval += dataTmp->interval;
    } else if (topicName == "net:napi_gro_receive_entry") {
        UpdateQueueData(dataTmp->pmuData, dataTmp->len);
    } else if (topicName == "cycles") {
        UpdateCyclesData(dataTmp->pmuData, dataTmp->len);
    }
}

void NetHardIrq::UpdateEnvInfo(const DataList &dataList)
{
    std::string instanceName = dataList.topic.instanceName;
    std::string topicName = dataList.topic.topicName;
    const EnvCpuUtilParam *dataTmp = static_cast<EnvCpuUtilParam *>(dataList.data[0]);
    if (topicName == "cpu_util") {
        UpdateCpuInfo(dataTmp);
    }
}

void UpdateNetBaseInfo(std::unordered_map<std::string, EthQueInfo> &ethQueData,
    std::unordered_map<uint32_t, std::string> &ifIdxToName, const NetworkInterfaceData *data, int dataLen)
{
    for (int i = 0; i < dataLen; i++) {
        std::string dev = std::string(data[i].name);
        ethQueData[dev].dev = dev;
        ethQueData[dev].operstate = data[i].operstate;
        ifIdxToName[data[i].ifindex] = dev;
    }
}

void UpdateNetDriverInfo(std::unordered_map<std::string, EthQueInfo> &ethQueData,
    const NetworkInterfaceDriverData *data, int dataLen)
{
    for (int i = 0; i < dataLen; i++) {
        const std::string dev = std::string(data[i].name);
        ethQueData[dev].dev = dev;
        ethQueData[dev].busInfo = std::string(data[i].busInfo);
        ethQueData[dev].driver = std::string(data[i].driver);
    }
}

void UpdateThreadNumaInfo(std::unordered_map<uint32_t, ThreadNumaInfo> &threadNumaInfo,
    const NetThreadQueData *data, int dataLen)
{
    for (int i = 0; i < dataLen; ++i) {
        int tid = data[i].tid;
        auto &tmp = threadNumaInfo[tid];
        tmp.queRxTimes[data[i].ifIndex][data[i].queueId] += data[i].times;
    }
}

void NetHardIrq::UpdateNetIntfInfo(const DataList &dataList)
{
    std::string instanceName = dataList.topic.instanceName;
    if (instanceName != OE_NET_INTF_INFO) {
        return;
    }
    const std::string topicName = dataList.topic.topicName;
    const std::string params = dataList.topic.params;
    if (topicName == OE_NETWORK_INTERFACE_BASE_TOPIC) {
        const NetIntfBaseDataList *dataTmp = static_cast<NetIntfBaseDataList *>(dataList.data[0]);
        UpdateNetBaseInfo(ethQueData, ifIdxToName, dataTmp->base, dataTmp->count);
    } else if (topicName == OE_NETWORK_INTERFACE_DRIVER_TOPIC) {
        const NetIntfDriverDataList *dataTmp = static_cast<NetIntfDriverDataList *>(dataList.data[0]);
        UpdateNetDriverInfo(ethQueData, dataTmp->driver, dataTmp->count);
    } else if (topicName == OE_NET_THREAD_QUE_DATA && params == OE_PARA_THREAD_RECV_QUE_CNT) {
        const NetThreadQueDataList *dataTmp = static_cast<NetThreadQueDataList *>(dataList.data[0]);
        UpdateThreadNumaInfo(threadNumaInfo, dataTmp->queData, dataTmp->count);
    }
}

void NetHardIrq::UpdateCpuInfo(const EnvCpuUtilParam *data)
{
    if (data == nullptr) {
        return;
    }
    if (data->dataReady == ENV_DATA_NOT_READY) {
        return;
    }
    size_t cpuNumConfig = data->cpuNumConfig + 1;
    if (cpuTimeDiff.size() < cpuNumConfig) {
        cpuTimeDiff.resize(cpuNumConfig);
        cpuUtil.resize(cpuNumConfig);
        for (size_t i = 0; i < cpuTimeDiff.size(); i++) {
            cpuTimeDiff[i].resize(CPU_UTIL_TYPE_MAX, 0);
            cpuUtil[i].resize(CPU_UTIL_TYPE_MAX, 0);
        }
    }
    for (size_t i = 0; i < cpuTimeDiff.size(); ++i) {
        for (size_t j = 0; j < cpuTimeDiff[i].size(); ++j) {
            cpuTimeDiff[i][j] = data->times[i][j];
        }
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

void NetHardIrq::UpdateCyclesData(const PmuData *data, int dataLen)
{
    if (!envInit) {
        return;
    }
    for (int i = 0; i < dataLen; ++i) {
        int tid = data[i].tid;
        auto &tmpData = threadNumaInfo[tid];
        if (tmpData.cyclesNumaRatio.empty()) {
            tmpData.cyclesNumaRatio.resize(numaNum, 0);
        }
        int numa = cpu2Numa[data[i].cpu];
        tmpData.cyclesNumaRatio[numa] += data[i].period;
        tmpData.cyclesSum += data[i].period;
    }
}

void NetHardIrq::AddIrqToQueueInfo()
{
    for (auto &item : netQueue) {
        const std::string &dev = item.first;
        if (ethQueData.count(dev) == 0) {
            continue;
        }
        for (auto &que : item.second) {
            const int &queueId = que.first;
            auto &queInfo = que.second;
            if (ethQueData[dev].que2Irq.count(queueId) == 0) {
                continue;
            }
            queInfo.irqId = ethQueData[dev].que2Irq[queueId];
            queInfo.queueId = queueId;
        }
    }
}

void NetHardIrq::UpdateThreadAndQueueInfo(const RecNetQueue &queData, const RecNetThreads &thrData)
{
    int queueId = queData.queueMapping;
    int core = thrData.core;
    int node = cpu2Numa[core];
    QueueInfo &info = netQueue[queData.dev][queueId];
    size_t size = info.numaRxTimes.size();
    if (size < numaNum) {
        info.numaRxTimes.resize(numaNum, 0);
    }
    info.numaRxTimes[node]++;
}

void NetHardIrq::ClearInvalidQueueInfo()
{
    for (auto devItem = netQueue.begin(); devItem != netQueue.end(); ) {
        auto &queueMap = devItem->second;
        for (auto queueItem = queueMap.begin(); queueItem != queueMap.end(); ) {
            if (queueItem->second.irqId == INVAILD_IRQ_ID) {
                AddLog([&]() {
                    return "ClearInvalidQueueInfo" + devItem->first +
                        ": " + std::to_string(queueItem->second.queueId);
                    });
                queueItem = queueMap.erase(queueItem);
            } else {
                ++queueItem;
            }
        }

        if (queueMap.empty()) {
            devItem = netQueue.erase(devItem);
        } else {
            ++devItem;
        }
    }
}

void NetHardIrq::CalCpuUtil()
{
    for (size_t cpu = 0; cpu < cpuUtil.size(); ++cpu) {
        for (int type = 0; type < CPU_UTIL_TYPE_MAX; ++type) {
            cpuUtil[cpu][type] = cpuTimeDiff[cpu][CPU_TIME_SUM] == 0 ?
                0.0 : cpuTimeDiff[cpu][type] * 100.0 / cpuTimeDiff[cpu][CPU_TIME_SUM];
        }
    }
}

void NetHardIrq::TunePreprocessing()
{
    for (auto &devItem : netQueue) {
        for (auto &queItem : devItem.second) {
            auto &info = queItem.second;
            int maxNode = 0, maxTimes = 0;
            info.rxSum = 0;
            for (size_t i = 0; i < info.numaRxTimes.size(); ++i) {
                if (maxTimes <= info.numaRxTimes[i]) {
                    maxTimes = info.numaRxTimes[i];
                    maxNode = i;
                }
                info.rxSum += info.numaRxTimes[i];
            }
            info.rxMaxNode = maxNode;
        }
    }
}

void NetHardIrq::MigrateHardIrq()
{
    std::vector<HardIrqMigUint> migUint;
    for (auto &devItem : netQueue) {
        const std::string &dev = devItem.first;
        for (auto &queItem : devItem.second) {
            auto &info = queItem.second;
            if (info.rxSum < NET_RXT_THRESHOLD) {
                continue;
            }
            HardIrqMigUint unit;
            unit.irqId = info.irqId;
            unit.rxSum = info.rxSum;
            unit.queId = info.queueId;
            unit.dev = dev;
            unit.preferredNode = info.rxMaxNode;
            unit.lastBindCore = info.lastBindCpu;
            unit.priority = unit.lastBindCore == INVALID_CPU_ID ? 0 : cpu2Numa[unit.lastBindCore] == unit.preferredNode;
            migUint.emplace_back(unit);
        }
    }
    // sort by rxSum
    std::sort(migUint.begin(), migUint.end(), [](const HardIrqMigUint &a, const HardIrqMigUint &b) {
        if (a.priority != b.priority) {
            return a.priority > b.priority; // perferred irq which last period has bound
        }
        return a.rxSum > b.rxSum;
        });

    std::vector<std::vector<CpuSort>> cpuSort = SortNumaCpuUtil();
    AddLog([&]() { return CpuSortLog(cpuSort); });
    std::vector<int> newBindCnt(numaNum);
    for (auto &unit : migUint) {
        int preferredNode = unit.preferredNode;
        AddLog([&]() {
            return "migUint " + unit.GetInfo();
            });
        if (unit.lastBindCore != -1 && preferredNode == cpu2Numa[unit.lastBindCore]) {
            AddLog([]() { return " skip\n"; });
            continue;
        }
        const std::vector<CpuSort> &sortedList = cpuSort[preferredNode];
        /* 1. different interrupts should be allocated to different cores as much as possible
           2. interrupt priority binding on cores with low CPU utilization */
        int preferredCpu = sortedList[newBindCnt[preferredNode] % numaNum].cpu; // this may cause bind to the same core
        int err = IrqSetSmpAffinity(unit.irqId, std::to_string(preferredCpu));
        if (err) {
            WARN(logger, "MigrateHardIrq set irq affinity failed, irqId: " << unit.irqId << ", preferredCpu: "
                << preferredCpu << ", err: " << err);
        } else {
            newBindCnt[preferredNode]++;
            AddLog([&]() {
                return ", set irq from core " + (unit.lastBindCore == -1 ?
                    irqInfo[unit.irqId].originAffinity : std::to_string(unit.lastBindCore))
                    + " to " + std::to_string(preferredCpu)
                    + ", numaBindCnt " + std::to_string(newBindCnt[preferredNode]) + " \n";
                });
            netQueue[unit.dev][unit.queId].lastBindCpu = preferredCpu;
            irqInfo[unit.irqId].isTuned = true;
            irqInfo[unit.irqId].lastAffinity = preferredCpu;
        }
    }
}

void NetHardIrq::ResetCpuInfo()
{
    for (auto &cpuItem : cpuTimeDiff) {
        for (auto &item : cpuItem) {
            item = 0;
        }
    }
}

void oeaware::NetHardIrq::ResetNetQueue()
{
    for (auto &devItem : netQueue) {
        for (auto &queItem : devItem.second) {
            auto &info = queItem.second;
            for (auto &rx : info.numaRxTimes) {
                rx = 0;
            }
        }
    }
}

std::vector<std::vector<CpuSort>> NetHardIrq::SortNumaCpuUtil()
{
    std::vector<std::vector<CpuSort>> rst;
    rst.resize(numaNum);
    for (size_t cpu = 0; cpu < cpu2Numa.size(); ++cpu) {
        int nid = cpu2Numa[cpu];
        // 100.0 is the max cpu util
        rst[nid].emplace_back(cpu, 100.0 - cpuUtil[cpu][CPU_IDLE] - cpuUtil[cpu][CPU_IRQ] - cpuUtil[cpu][CPU_SOFTIRQ]);
    }
    for (auto &nodeCpuSort : rst) {
        std::sort(nodeCpuSort.begin(), nodeCpuSort.end(), [](const CpuSort &a, const CpuSort &b) {
            return a.ratio < b.ratio;
        });
    }
    return rst;
}

void NetHardIrq::Tune()
{
    AddIrqToQueueInfo();
    ClearInvalidQueueInfo();
    TunePreprocessing();
    MigrateHardIrq();
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
    AddLog([&]() {
        return "thread data cnt:" + std::to_string(threadData.size()) +
            ", queue data cnt:" + std::to_string(queueData.size()) +
            ", match times:" + std::to_string(matchTimes) + "\n";
        });
    threadData.clear();
    queueData.clear();
}

void NetHardIrq::MatchThreadQueAndNuma()
{
    for (const auto &thr : threadNumaInfo) {
        const auto &data = thr.second;
        if (data.queRxTimes.empty()) {
            continue; // skip threads without net packets recv
        }
        std::vector<double> ratio(numaNum);
        AddLog([&]() {
            return "MatchThreadQueAndNuma: thread:" + std::to_string(thr.first) +
                ", cycles: " + std::to_string(thr.second.cyclesSum) + ", numa ratio: ";
            });
        for (size_t i = 0; i < thr.second.cyclesNumaRatio.size(); i++) {
            ratio[i] = (double)thr.second.cyclesNumaRatio[i] / thr.second.cyclesSum;
            AddLog([&]() { return std::to_string(ratio[i]) + " "; });
        }
        AddLog([&]() { return "\n"; });
        for (const auto &devData : data.queRxTimes) {
            const auto ifIndex = devData.first;
            if (ifIdxToName.count(ifIndex) == 0 || ifIdxToName[ifIndex].empty()) {
                continue; // skip invalid ifIndex
            }
            const auto devName = ifIdxToName[ifIndex];
            for (const auto &queData : devData.second) {
                auto queId = queData.first;
                uint64_t rxTimes = queData.second;
                QueueInfo &info = netQueue[devName][queId];
                if (info.numaRxTimes.size() < numaNum) {
                    info.numaRxTimes.resize(numaNum, 0);
                }
                AddLog([&]() {
                    return "MatchThreadQueAndNuma: " + devName + ", queId " + std::to_string(queId) +
                        ", rxTimes " + std::to_string(rxTimes) + ", Numa Rx: ";
                    });
                for (size_t n = 0; n < numaNum; ++n) {
                    info.numaRxTimes[n] += rxTimes * ratio[n];
                    AddLog([&]() { return std::to_string(info.numaRxTimes[n]) + " "; });
                }
                AddLog([&]() { return "\n"; });
            }
        }
    }
    AddLog([&]() {
        return "MatchThreadQueAndNuma: threadNumaInfo size "
            + std::to_string(threadNumaInfo.size()) + " netQueue size " + std::to_string(netQueue.size()) + "\n";
        });
}

void NetHardIrq::UpdateData(const DataList &dataList)
{
    UpdateSystemInfo(dataList);
    UpdatePmuSampleInfo(dataList);
    UpdateEnvInfo(dataList);
    UpdateNetIntfInfo(dataList);
}

oeaware::Result NetHardIrq::Enable(const std::string &param)
{
    bool isActive;
    irqbalanceStatus = false;
    Init();
    if (!ResolveCmd(param)) {
        return oeaware::Result(FAILED, "NetHardIrq resolve cmd failed \n" + GetHelp());
    }
    if (!conf.InitConf(HARDIRQ_CONFIG_PATH)) {
        return oeaware::Result(FAILED, "read " + HARDIRQ_CONFIG_PATH + " failed.");
    }

    // irq tune confilct with irqbalance
    if (ServiceIsActive("irqbalance", isActive))
    {
        irqbalanceStatus = isActive;
    }
    if (irqbalanceStatus) {
        ServiceControl("irqbalance", "stop");
        INFO(logger, "irqbalance stop when NetHardIrq enable");
    }
    subscribeTopics.clear();
    subscribeTopics.emplace_back(oeaware::Topic{ OE_ENV_INFO, "static", "" });
    subscribeTopics.emplace_back(oeaware::Topic{ OE_ENV_INFO, "cpu_util", "" });
    subscribeTopics.emplace_back(oeaware::Topic{ OE_NET_INTF_INFO, OE_NETWORK_INTERFACE_BASE_TOPIC, "operstate_up" });
    subscribeTopics.emplace_back(oeaware::Topic{ OE_NET_INTF_INFO, OE_NETWORK_INTERFACE_DRIVER_TOPIC, "operstate_up" });
    if (highNoiseSample) {
        subscribeTopics.emplace_back(oeaware::Topic{ OE_PMU_SAMPLING_COLLECTOR, "skb:skb_copy_datagram_iovec", "" });
        subscribeTopics.emplace_back(oeaware::Topic{ OE_PMU_SAMPLING_COLLECTOR, "net:napi_gro_receive_entry", "" });
    } else {
        subscribeTopics.emplace_back(oeaware::Topic{ OE_NET_INTF_INFO, OE_NET_THREAD_QUE_DATA,
            OE_PARA_THREAD_RECV_QUE_CNT });
        subscribeTopics.emplace_back(oeaware::Topic{ OE_PMU_SAMPLING_COLLECTOR, "cycles", "" });
    }
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }

    debugLog += " highNoiseSample is " + std::to_string(highNoiseSample) + "\n";
    return oeaware::Result(OK);
}

void NetHardIrq::Disable()
{
    if (irqbalanceStatus) {
        INFO(logger, "irqbalance start when NetHardIrq disbale");
        ServiceControl("irqbalance", "start");
    }
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    for (const auto &info : irqInfo) {
        if (!info.second.isTuned) {
            continue;
        }
        int err = IrqSetSmpAffinity(info.first, info.second.originAffinity);
        INFO(logger, "Restore IRQ " << info.first << " affinity from "
            << info.second.lastAffinity << " to " << info.second.originAffinity << ", result " << strerror(err));
    }
    envInit = false;
    cpu2Numa.clear();
    irqInfo.clear();
    netQueue.clear();
    subscribeTopics.clear();
    highNoiseSample = false;
}

void NetHardIrq::PublishData()
{
    if (!debugTopicOpen) {
        return;
    }
    DataList dataList;
    if (debugTopicOpen) {
        oeaware::SetDataListTopic(&dataList, name, OE_TOPIC_NET_HIRQ_TUNE_DEBUG_INFO, "");
        NetHirqTuneDebugInfo *debugInfo = new NetHirqTuneDebugInfo();
        debugInfo->log = new char[debugLog.size() + 1];
        strcpy_s(debugInfo->log, debugLog.size() + 1, debugLog.data());
        dataList.len = 1;
        dataList.data = new void *[1];
        dataList.data[0] = debugInfo;
        Publish(dataList);
    }
}

std::string NetHardIrq::CpuSortLog(const std::vector<std::vector<CpuSort>> &cpuSort)
{
    std::string log;
    for (size_t n = 0; n < cpuSort.size(); ++n) {
        log += "NUMA " + std::to_string(n) + ": ";
        for (auto &cpuSort : cpuSort[n]) {
            std::ostringstream oss;
            oss << "[ " << cpuSort.cpu << "," << std::fixed << std::setprecision(1) << cpuSort.ratio << "]";
            log += oss.str();
        }
        log += "\n";
    }
    return log;
}

void NetHardIrq::Run()
{
    AddLog([&]() {
        return "======================================== NetHardIrq run "
            + std::to_string(runCnt) + " ========================================\n";
        });
    runCnt++;
    if (!envInit) {
        return;
    }
    CalCpuUtil();
    if (highNoiseSample) {
        MatchThreadAndQueue();
    } else {
        MatchThreadQueAndNuma();
    }
    conf.GetEthQueData(ethQueData, conf.GetQueRegex());
    Tune();
    ResetNetQueue();
    ResetCpuInfo();
    PublishData();
    netDataInterval = 0;
    ethQueData.clear();
    if (showVerbose) {
        INFO(logger, debugLog);
    }
    debugLog = "";
    threadNumaInfo.clear();
}

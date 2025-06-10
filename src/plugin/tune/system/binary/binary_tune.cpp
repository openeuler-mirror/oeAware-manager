/******************************************************************************
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
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
#include <unistd.h>
#include <elf.h>
#include <cstdlib>
#include <fcntl.h>
#include <yaml-cpp/yaml.h>
#include "oeaware/data/env_data.h"
#include "oeaware/data/thread_info.h"
#include "oeaware/data/docker_data.h"
#include "binary_tune.h"

bool IsSmtEnable()
{
    std::string content;
    std::ifstream inFile("/sys/devices/system/cpu/smt/active");
    if (!inFile.is_open()) {
        return false;
    }
    inFile >> content;
    inFile.close();
    return content == "1";
}

bool BinaryTune::ReadConfig(const std::string &path)
{
    std::ifstream configFile(path);
    if (!configFile.is_open()) {
        WARN(logger, "binary_tune.yaml open failed.");
        return false;
    }
    YAML::Node node = YAML::LoadFile(path);
    if (!node.IsMap()) {
        WARN(logger, "binary_tune.yaml format error.");
        return false;
    }
    for (auto item : node) {
        std::string name = item.first.as<std::string>();
        try {
            auto policy = item.second.as<int32_t>();
            configBinary[name] = policy;
        } catch (...) {
            WARN(logger, "Value for binary '" << name << "' is not a valid integer.");
            return false;
        }
    }
    return true;
}

BinaryTune::BinaryTune()
{
    name = OE_BINARY_TUNE;
    description = "Bind the special binary running in the container to the physical core.";
    version = "1.0.0";
    period = 1000;
    priority = 2;
    type = oeaware::TUNE;
    oeaware::Topic topic;
    topic.instanceName = this->name;
    topic.topicName = this->name;
    supportTopics.push_back(topic);
    subscribeTopics.emplace_back(oeaware::Topic{OE_ENV_INFO, "static", "" });
    subscribeTopics.emplace_back(oeaware::Topic{OE_ENV_INFO, "realtime", "" });
    subscribeTopics.emplace_back(oeaware::Topic{OE_THREAD_COLLECTOR, OE_THREAD_COLLECTOR, ""});
    subscribeTopics.emplace_back(oeaware::Topic{OE_DOCKER_COLLECTOR, OE_DOCKER_COLLECTOR, ""});
}

oeaware::Result BinaryTune::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void BinaryTune::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void BinaryTune::UpdateData(const DataList &dataList)
{
    std::string topicName  = dataList.topic.topicName;
    if (topicName == "static" && !envInit) {
        auto *dataTmp = static_cast<EnvStaticInfo *>(dataList.data[0]);
        numaNum = dataTmp->numaNum;
        cpuCoreNum = dataTmp->cpuNumConfig;
        cpu2Numa.resize(dataTmp->cpuNumConfig);
        phyNumaMask.resize(numaNum);
        for (int i = 0; i < numaNum; i++) {
            CPU_ZERO(&phyNumaMask[i]);
        }
        for (int i = 0; i < dataTmp->cpuNumConfig; i++) {
            cpu2Numa[i] = dataTmp->cpu2Node[i];
            if (i % 2 == 0) {
                CPU_SET(i, &phyNumaMask[cpu2Numa[i]]);
            }
        }
        envInit = true;
        Unsubscribe(oeaware::Topic{ OE_ENV_INFO, "static", "" }); // todo define a var
        return;
    }
    if (!envInit) {
        return;
    }
    if (topicName == "realtime") {
        auto *dataTmp = static_cast<EnvRealTimeInfo *>(dataList.data[0]);
        if (dataTmp->dataReady == ENV_DATA_READY) {
            cpuNumOnline = dataTmp->cpuNumOnline;
        } else {
            cpuNumOnline = cpuCoreNum;  // if data is not ready, assume that all cpus are online temporary
        }
    }
    if (topicName == OE_DOCKER_COLLECTOR) {
        containers.clear();
        for (unsigned long long i = 0; i < dataList.len; i++) {
            auto* container = static_cast<Container*>(dataList.data[i]);
            containers.emplace_back(container->id,
                                    container->cfs_period_us,
                                    container->cfs_quota_us,
                                    container->cfs_burst_us,
                                    container->cpus,
                                    container->tasks);
        }
        return;
    }
    if (topicName == OE_THREAD_COLLECTOR) {
        threads.clear();
        for (unsigned long long i = 0; i < dataList.len; i++) {
            auto* thread = static_cast<ThreadInfo*>(dataList.data[i]);
            threads[thread->tid] = thread->name;
        }
        return;
    }
}

oeaware::Result BinaryTune::Enable(const std::string &param)
{
    (void)param;
    if (!IsSmtEnable()) {
        WARN(logger, "smt is not enable, open binary tune failed.");
        return oeaware::Result(FAILED);
    }
    configBinary.clear();
    if (!ReadConfig(configPath)) {
        configBinary.clear();
        WARN(logger, "read " << configPath << " failed. run without config.");
    }
    for (auto &topic : subscribeTopics) {
        Subscribe(topic);
    }
    return oeaware::Result(OK);
}

void BinaryTune::Disable()
{
    for (auto &topic : subscribeTopics) {
        Unsubscribe(topic);
    }
    for (auto it : tuneThreads) {
        cpu_set_t *oldMask = &(it.second);
        sched_setaffinity(it.first, sizeof(*oldMask), oldMask);
    }
    tuneThreads.clear();
    hasLoggedThreads.clear();
}

void BinaryTune::ParseBinaryElf(const std::string &filePath, int32_t &policy)
{
    size_t separatorPos = filePath.find_last_of("/\\");
    if (separatorPos != std::string::npos) {
        std::string binaryName = filePath.substr(separatorPos + 1);
        if (configBinary.count(binaryName) != 0) {
            policy = configBinary[binaryName];
            return;
        }
    }

    policy = -1;
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd == -1) {
        return;
    }
    Elf64_Ehdr ehdr;
    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
        close(fd);
        return;
    }
    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        close(fd);
        return;
    }
    Elf64_Off shoff = ehdr.e_shoff;
    size_t shentsize = ehdr.e_shentsize;
    uint16_t shnum = ehdr.e_shnum;
    uint16_t shstrndx = ehdr.e_shstrndx;

    Elf64_Shdr shstrtabShdr;
    auto readFromElf = [&fd](unsigned long offset, size_t buffLen, void* buff) -> bool {
        if (lseek(fd, (long)offset, SEEK_SET) == -1) {
            return false;
        }
        if (read(fd, buff, buffLen) != (ssize_t)buffLen) {
            return false;
        }
        return true;
    };
    if (!readFromElf(shoff + shstrndx * shentsize, sizeof(shstrtabShdr), &shstrtabShdr)) {
        close(fd);
        return;
    }

    char* shstrtab = (char*)malloc(shstrtabShdr.sh_size);
    if (!shstrtab) {
        close(fd);
        return;
    }
    if (!readFromElf(shstrtabShdr.sh_offset, shstrtabShdr.sh_size, shstrtab)) {
        free(shstrtab);
        close(fd);
        return;
    }

    for (uint16_t i = 0; i < shnum; i++) {
        Elf64_Shdr shdr;
        if (!readFromElf(shoff + i * shentsize, sizeof(shdr), &shdr)) {
            free(shstrtab);
            close(fd);
            return;
        }
        const char *name = shstrtab + shdr.sh_name;
        if (strcmp(name, ".GCC4OE_oeAware") == 0 || strcmp(name, ".LLVM4OE_oeAware") == 0) {
            if (shdr.sh_size == sizeof(policy)) {
                if (!readFromElf(shdr.sh_offset, shdr.sh_size, &policy)) {
                    free(shstrtab);
                    close(fd);
                    return;
                }
            }
            break;
        }
    }
    free(shstrtab);
    close(fd);
}

void BinaryTune::BindAllCores(const std::vector<int32_t> &threads)
{
    cpu_set_t newMask;
    CPU_ZERO(&newMask);
    for (int32_t i = 0; i < cpuCoreNum; i++) {
        CPU_SET(i, &newMask);
    }
    for (auto t : threads) {
        SetAffinity(t, newMask);
    }
}

void BinaryTune::FindSpecialBin(std::map<int32_t, int64_t> &dockerCpuOnNuma,
                                std::map<int32_t, std::vector<int32_t>> &threadWaitTuneByNuma,
                                std::map<int32_t, std::vector<std::pair<int32_t, std::vector<int32_t>>>> &threadWaitTuneByCore)
{
    std::map<std::string, int32_t> threadPolicy;
    std::set<int32_t> containerThreads;
    int32_t buffLen = 1024;
    char* filePathBuff = new char[buffLen];
    for (auto &i : containers) {
        std::vector<int> cpus = oeaware::ParseRange(i.cpus);
        bool isFullNuma = true; // true means the container's cpu is bound to all cpus of a single numa
        int32_t initNuma = cpu2Numa[cpus[0]];
        for (auto &cpu : cpus) {
            if (cpu2Numa[cpu] != initNuma) {
                initNuma = -1;
                isFullNuma = false;
                break;
            }
        }
        if (cpus.size() != static_cast<size_t>(cpuCoreNum / numaNum)) {
            isFullNuma = false;
        }

        threadPolicy.clear();
        containerThreads.insert(i.tasks.begin(), i.tasks.end());
        std::vector<int32_t> threadBindCore;
        std::vector<int32_t> crossNumaThreads;

        for (auto j : i.tasks) {
            if (threads.count(j) == 0) {
                continue;
            }
            if (threadPolicy.count(threads[j]) == 0) {
                std::string procExe = "/proc/"+std::to_string(j)+"/exe";
                ssize_t bytes_read = readlink(procExe.c_str(), filePathBuff, buffLen - 1);
                if (bytes_read == -1) {
                    continue;
                }
                filePathBuff[bytes_read] = '\0';
                std::string filePath(filePathBuff);
                int32_t policy = -1;
                ParseBinaryElf(filePath, policy);
                threadPolicy[threads[j]] = policy;
            }

            // if isFullNuma is false, the plugin will not take effect and should record these threads to print log
            if (!isFullNuma) {
                if ((threadPolicy[threads[j]] == 1 || threadPolicy[threads[j]] == 2)) {
                    crossNumaThreads.push_back(j);  // record user-configured threads
                    if (hasLoggedThreads.find(j) == hasLoggedThreads.end()) {   // this log is only printed once
                        WARN(logger, "binary_tune: container is not bind on the whole numa, so donot bind thread " << j);
                        hasLoggedThreads.insert(j);
                    }
                }
                continue;
            }

            if (threadPolicy[threads[j]] == 1) {  // 1 means all threads bind on all physical cores.
                threadWaitTuneByNuma[initNuma].push_back(j);
                hasLoggedThreads.erase(j);
                continue;
            }
            if (threadPolicy[threads[j]] == 2) {  // 2 means all threads bind on some physical cores.
                threadBindCore.push_back(j);
                hasLoggedThreads.erase(j);
                continue;
            }
        }

        if (isFullNuma) {
            dockerCpuOnNuma[initNuma] += i.cfsQuotaUs/i.cfsPeriodUs;
            if (!threadBindCore.empty()) {
                threadWaitTuneByCore[initNuma].emplace_back(std::make_pair(i.cfsQuotaUs / i.cfsPeriodUs, std::move(threadBindCore)));
            }
        } else if (!crossNumaThreads.empty()) {
            BindAllCores(crossNumaThreads); // restore thread-core binding to match the container's cpu affinity
        }
    }
    delete[] filePathBuff;
    // clear dead thread
    for (auto it = tuneThreads.begin(); it != tuneThreads.end();) {
        if (containerThreads.count(it->first) == 0) {
            it = tuneThreads.erase(it);
            continue;
        }
        ++it;
    }

    for (auto it = hasLoggedThreads.begin(); it != hasLoggedThreads.end();) {
        if (containerThreads.count(*it) == 0) {
            it = hasLoggedThreads.erase(it);
        } else {
            ++it;
        }
    }
}

void BinaryTune::SetAffinity(int32_t tid, cpu_set_t& newMask)
{
    cpu_set_t currentMask;
    sched_getaffinity(tid, sizeof(currentMask), &currentMask);
    if (CPU_EQUAL(&newMask, &currentMask)) {
        return;
    }
    if (sched_setaffinity(tid, sizeof(newMask), &newMask) == 0) {
        tuneThreads.emplace(tid, currentMask);
    }
}

void BinaryTune::BindCpuCore(int32_t numaNode, std::map<int32_t,
                             std::vector<std::pair<int32_t, std::vector<int32_t>>>> &threadWaitTuneByCore)
{
    int32_t coreIndex = numaNode * (cpuCoreNum/numaNum);
    int32_t endIndex = (numaNode + 1) * (cpuCoreNum/numaNum);
    for (auto &item : threadWaitTuneByCore[numaNode]) {
        int32_t bindCoreNum = item.first;
        cpu_set_t newMask;
        CPU_ZERO(&newMask);
        for (;coreIndex < endIndex && bindCoreNum > 0; coreIndex+=2) {
            CPU_SET(coreIndex, &newMask);
            bindCoreNum--;
        }
        if (bindCoreNum > 0) {
            continue;
        }
        for (auto t : item.second) {
            SetAffinity(t, newMask);
        }
    }
}

void BinaryTune::BindNuma(int32_t numaNode, std::map<int32_t, std::vector<int32_t>> &threadWaitTuneByNuma)
{
    for (auto t : threadWaitTuneByNuma[numaNode]) {
        SetAffinity(t, phyNumaMask[numaNode]);
    }
}

void BinaryTune::Run()
{
    if (cpuNumOnline != cpuCoreNum) {
        if (isCpuAllOnline) {   // this log is only printed once when cpu is found to be offline for the first time
            WARN(logger, "binary_tune: plugin will not take effect because not all cpus are online");
            isCpuAllOnline = false;
        }
        return;
    }
    isCpuAllOnline = true;

    std::map<int32_t, std::vector<int32_t>> threadWaitTuneByNuma;
    std::map<int32_t, std::vector<std::pair<int32_t, std::vector<int32_t>>>> threadWaitTuneByCore;
    std::map<int32_t, int64_t> dockerCpuOnNuma;
    FindSpecialBin(dockerCpuOnNuma, threadWaitTuneByNuma, threadWaitTuneByCore);
    for (auto &i : dockerCpuOnNuma) {
        if (i.second > cpuCoreNum/numaNum/2) {
            for (auto it : tuneThreads) {
                cpu_set_t *oldMask = &(it.second);
                sched_setaffinity(it.first, sizeof(*oldMask), oldMask);
            }
            tuneThreads.clear();
            continue;
        }
        BindCpuCore(i.first, threadWaitTuneByCore);
        BindNuma(i.first, threadWaitTuneByNuma);
    }
}

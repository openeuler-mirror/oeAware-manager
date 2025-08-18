#include "hwprobe_analysis.h"
#include <fstream>
#include <regex>
#include <sys/syscall.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <asm/hwprobe.h>
#include <map>
#include <vector>

namespace oeaware {

std::string to_hex(uint64_t v) {
    std::ostringstream oss;
    oss << std::hex << v;
    return oss.str();
}

bool HwprobeAnalysis::CheckHwprobeHeader() {
    std::ifstream file("/usr/include/asm/hwprobe.h");
    return file.good();
}

std::vector<HwprobeAnalysis::DynamicKey> HwprobeAnalysis::ParseHwprobeKeys() {
    std::vector<HwprobeAnalysis::DynamicKey> keys;
    std::ifstream file("/usr/include/asm/hwprobe.h");
    if (!file) {
        return keys;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::regex key_pattern(R"(#define\s+(RISCV_HWPROBE_KEY_\w+)\s+(\d+|0x[0-9a-fA-F]+))");
    std::smatch matches;
    auto iter = content.cbegin();
    while (std::regex_search(iter, content.cend(), matches, key_pattern)) {
        try {
            int64_t key_value = std::stoll(matches[2].str(), nullptr, 0);
            keys.push_back({key_value, matches[1].str()});
        } catch (...) {
            // 如果键值转换失败（如非数字字符串），跳过此键
            // 因为hwprobe.h可能包含非键值定义的宏，我们只关心有效的键
        }
        iter = matches.suffix().first;
    }
    return keys;
}

bool HwprobeAnalysis::ProbeHwInfo(std::vector<HwprobeAnalysis::DynamicKey>& keys, std::vector<riscv_hwprobe>& pairs) {
    pairs.clear();
    for (const auto& dk : keys) {
        pairs.push_back({dk.key, 0});
    }
    long ret = syscall(SYS_riscv_hwprobe, pairs.data(), pairs.size(), 0, nullptr, 0);
    return ret == 0;
}

std::string HwprobeAnalysis::ParseHwprobeNote(int64_t key, uint64_t value) {
    switch (key) {
        case RISCV_HWPROBE_KEY_MVENDORID:
            return "Manufacturer ID ";
            
        case RISCV_HWPROBE_KEY_MARCHID:
            return "Architecture ID";
            
        case RISCV_HWPROBE_KEY_MIMPID:
            return "Implementation ID";
            
        case RISCV_HWPROBE_KEY_BASE_BEHAVIOR: {
            std::string desc = "Base behavior: ";
            if (value & RISCV_HWPROBE_BASE_BEHAVIOR_IMA)
                desc += "IMA (Integer, Multiply/Divide, Atomic) ";
            return desc;
        }
            
        case RISCV_HWPROBE_KEY_IMA_EXT_0: {
            const std::map<uint64_t, std::string> extension_map = {
                {RISCV_HWPROBE_IMA_FD, "F/D (Single/Double Float)"},
                {RISCV_HWPROBE_IMA_C, "C (Compressed)"},
                {RISCV_HWPROBE_IMA_V, "V (Vector)"},
                {RISCV_HWPROBE_EXT_ZBA, "Zba (Address Generation)"},
                {RISCV_HWPROBE_EXT_ZBB, "Zbb (Basic Bit-Manipulation)"},
                {RISCV_HWPROBE_EXT_ZBS, "Zbs (Single-Bit Operations)"}
            };
            std::string desc = "IMA extensions: ";
            for (const auto& [mask, name] : extension_map) {
                if (value & mask) {
                    desc += name + " ";
                }
            }
            return desc;
        }
            
        case RISCV_HWPROBE_KEY_CPUPERF_0: {
            const std::map<uint64_t, std::string> perf_map = {
                {RISCV_HWPROBE_MISALIGNED_UNKNOWN, "Unknown"},
                {RISCV_HWPROBE_MISALIGNED_EMULATED, "Emulated (very slow)"},
                {RISCV_HWPROBE_MISALIGNED_SLOW, "Slow (slower than byte accesses)"},
                {RISCV_HWPROBE_MISALIGNED_FAST, "Fast (faster than byte accesses)"},
                {RISCV_HWPROBE_MISALIGNED_UNSUPPORTED, "Unsupported (will fault)"}
            };
            std::string desc = "Misaligned scalar access performance: ";
            auto it = perf_map.find(value);
            if (it != perf_map.end()) {
                desc += it->second;
            } else {
                desc += "Not specified";
            }
            return desc;
        }
            
        default:
            return "Unknown key value " + to_hex(key);
    }
}

HwprobeAnalysis::HwprobeAnalysis()
{
	name = OE_HWPROBE_ANALYSIS;
	period = ANALYSIS_TIME_PERIOD;
	priority = 1;
	type = SCENARIO;
	version = "1.0.0";
	for (auto &topic : topicStrs) {
		supportTopics.emplace_back(Topic{name, topic, ""});
	}
}

Result HwprobeAnalysis::Enable(const std::string &param)
{
	(void)param;
    if (!CheckHwprobeHeader()) {
        return Result(FAILED, "/usr/include/asm/hwprobe.h not found.");
    }
	return Result(OK);
}

void HwprobeAnalysis::Disable()
{
	AnalysisResultItemFree(&analysisResultItem);
	topicStatus.clear();
}

Result HwprobeAnalysis::OpenTopic(const oeaware::Topic &topic)
{
	if (std::find(topicStrs.begin(), topicStrs.end(), topic.topicName) == topicStrs.end()) {
		return Result(FAILED, "topic " + topic.topicName + " not support!");
	}
	auto topicType = topic.GetType();
	topicStatus[topicType].isOpen = true;
	topicStatus[topicType].beginTime = std::chrono::high_resolution_clock::now();
	auto paramsMap = GetKeyValueFromString(topic.params);
	if (paramsMap.count("t")) {
		topicStatus[topicType].time = atoi(paramsMap["t"].data());
	}
	return Result(OK);
}

void HwprobeAnalysis::CloseTopic(const oeaware::Topic &topic)
{
	auto topicType = topic.GetType();
	topicStatus[topicType].isOpen = false;
	topicStatus[topicType].isPublish = false;
}

void HwprobeAnalysis::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

void HwprobeAnalysis::PublishData(const Topic &topic)
{
	DataList dataList;
	SetDataListTopic(&dataList, topic.instanceName, topic.topicName, topic.params);
	dataList.len = 1;
	dataList.data = new void *[dataList.len];
	dataList.data[0] = &analysisResultItem;
	Publish(dataList, false);
}

void HwprobeAnalysis::Run()
{
	auto now = std::chrono::system_clock::now();
	for (auto &item : topicStatus) {
		auto &status = item.second;
		if (status.isOpen) {
			int curTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - status.beginTime).count();
			if (!status.isPublish && curTimeMs / MS_PER_SEC >= status.time) {
				auto topicType = item.first;
				const auto topic = Topic::GetTopicFromType(topicType);
				Analysis(topicType);
				PublishData(topic);
				status.isPublish = true;
			}
		}
	}
}

void HwprobeAnalysis::Analysis(const std::string &topicType)
{
    (void)topicType;

    std::vector<int> type;
    std::vector<std::vector<std::string>> metrics;

    auto keys = ParseHwprobeKeys();
    std::vector<riscv_hwprobe> pairs;
    if (!ProbeHwInfo(keys, pairs)) {
        type.emplace_back(DATA_TYPE_HWPROBE);
        metrics.emplace_back(std::vector<std::string>{"syscall", "failed", strerror(errno)});
    } else {
        for (size_t i = 0; i < pairs.size(); ++i) {
            if(pairs[i].value == 0) {
                continue;
            }
            std::string note = ParseHwprobeNote(pairs[i].key, pairs[i].value);
            type.emplace_back(DATA_TYPE_HWPROBE);
            metrics.emplace_back(std::vector<std::string>{
                keys[i].macro_name, 
                "0x" + to_hex(pairs[i].value), 
                note
            });
        }
    }
    std::string conclusion = "RISC-V hwprobe analysis finished.";
    std::vector<std::string> suggestionItem;
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
}

} // namespace oeaware

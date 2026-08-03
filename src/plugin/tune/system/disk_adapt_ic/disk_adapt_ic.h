/******************************************************************************
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/

#ifndef OEAWARE_DISK_ADAPT_IC_H
#define OEAWARE_DISK_ADAPT_IC_H

#include <string>
#include <map>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include "oeaware/interface.h"
#include "oeaware/data_list.h"

namespace oeaware {

constexpr const char* PLUGIN_NAME = "disk_adapt_ic";
constexpr const char* NVME_SYSFS_PATH = "/sys/block";
constexpr const char* NVME_DEV_PATH = "/dev";
constexpr const char* IC_CONFIG_FILE = "disk_adapt_ic.yaml";

constexpr int DEFAULT_SAMPLE_PERIOD_MS = 100;
constexpr int DEFAULT_IOPS_THRESHOLD = 1000000;
constexpr int DEFAULT_4K_RATIO_THRESHOLD = 50;
constexpr int AGGREGATION_ENABLE = 0x102;
constexpr int AGGREGATION_DISABLE = 0x0;
constexpr int MAX_RETRY_COUNT = 3;
constexpr int FUSE_DELAY_MS = 5000;

struct DiskStats {
    uint64_t read_ios = 0;
    uint64_t read_sectors = 0;
    uint64_t write_ios = 0;
    uint64_t write_sectors = 0;
    uint64_t ios = 0;
    uint64_t bytes = 0;
    uint64_t timestamp = 0;
};

struct DiskInfo {
    std::string name;
    std::string dev_path;
    std::string sysfs_path;
    bool is_nvme = false;
    bool in_polling_mode = false;
    bool is_enabled = true;
    bool ic_enabled = false;
    bool to_be_removed = false;
    int consecutive_high_load = 0;
    int consecutive_low_load = 0;
    DiskStats current_stats;
    DiskStats prev_stats;
    int error_count = 0;
    uint64_t last_fuse_time = 0;
    bool is_fused = false;
};

class DiskAdaptIC : public oeaware::Interface {
public:
    DiskAdaptIC();
    ~DiskAdaptIC() override=default;

    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param = "") override;
    void Disable() override;
    void Run() override;

private:
    bool LoadConfig();
    std::vector<std::string> DiscoverNvmeDisks();
    bool IsNvmeDisk(const std::string &disk_name);
    bool CheckDeviceExists(const std::string &dev_path);
    bool ReadIoPollMode(const std::string &sysfs_path);
    bool ReadDiskStats(const std::string &sysfs_path, DiskStats &stats);
    double CalculateIops(const DiskStats &current, const DiskStats &prev, uint64_t time_diff_ms);
    uint64_t CalculateAvgBlockSize(const DiskStats &current, const DiskStats &prev);
    bool Check4KBlockRatio(double avg_block_size_bytes);
    bool SetInterruptCoalescing(const std::string &dev_path, int value);
    bool GetInterruptCoalescing(const std::string &dev_path, int &value);
    void UpdateDiskStatus(DiskInfo &disk);
    void HandlePollingModeSwitch(DiskInfo &disk);
    void HandleDeviceError(DiskInfo &disk);
    void HandleDeviceRecovery(DiskInfo &disk);
    void AddDiskToMonitor(const std::string &disk_name);
    void RemoveDiskFromMonitor(const std::string &disk_name);

private:
    std::map<std::string, DiskInfo> monitored_disks_;
    std::mutex disks_mutex_;
    std::mutex config_mutex_;
    int iops_threshold_ = DEFAULT_IOPS_THRESHOLD;
    uint64_t sample_period_ms_ = DEFAULT_SAMPLE_PERIOD_MS;
    uint64_t last_run_time_ms_ = 0;
    const std::string config_path_ = oeaware::DEFAULT_PLUGIN_CONFIG_PATH + "/" + IC_CONFIG_FILE;
    bool config_loaded_ = false;
    bool is_enabled_ = false;
    
    uint64_t GetCurrentTimeMs() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    }
};

}

#endif // OEAWARE_DISK_ADAPT_IC_H
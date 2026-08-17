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

#include <fstream>
#include <sstream>
#include <iomanip>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/wait.h>
#include <cstring>
#include <yaml-cpp/yaml.h>
#include "disk_adapt_ic.h"
#include "oeaware/utils.h"

using namespace oeaware;

DiskAdaptIC::DiskAdaptIC()
{
    name = PLUGIN_NAME;
    description = "Dynamic disk interrupt coalescing based on IOPS load.";
    version = "1.0.0";
    period = 100;
    priority = 3;
    type = oeaware::TUNE;
}

/**
 * @brief 加载配置文件
 * 
 * 从配置文件中读取IOPS阈值和采样周期参数，
 * 配置文件不存在时使用默认值。
 * 
 * @return 加载成功返回true，失败返回false
 */
bool DiskAdaptIC::LoadConfig()
{
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::ifstream configFile(config_path_);
    if (!configFile.is_open()) {
        INFO(logger, "Config file " << config_path_ << " not found, using defaults.");
        config_loaded_ = true;
        return true;
    }

    try {
        YAML::Node node = YAML::LoadFile(config_path_);
        
        if (node["iops_threshold"]) {
            iops_threshold_ = node["iops_threshold"].as<int>();
            if (iops_threshold_ < 0 || iops_threshold_ > 10000000) {
                WARN(logger, "Invalid iops_threshold, using default: " << DEFAULT_IOPS_THRESHOLD);
                iops_threshold_ = DEFAULT_IOPS_THRESHOLD;
            }
            INFO(logger, "Loaded iops_threshold: " << iops_threshold_);
        }
        
        if (node["sample_period_ms"]) {
            sample_period_ms_ = node["sample_period_ms"].as<int>();
            if (sample_period_ms_ < 100 || sample_period_ms_ > 1000) {
                WARN(logger, "Invalid sample_period_ms, using default: " << DEFAULT_SAMPLE_PERIOD_MS);
                sample_period_ms_ = DEFAULT_SAMPLE_PERIOD_MS;
            }
            INFO(logger, "Loaded sample_period_ms: " << sample_period_ms_);
        }
        
        if (node["interrupt_coalescing_value"]) {
            std::string ival_str = node["interrupt_coalescing_value"].as<std::string>();
            try {
                std::size_t pos;
                int ival = std::stoi(ival_str, &pos, 0);  // 0 表示自动检测进制（0x开头为十六进制）
                if (pos == ival_str.size()) {
                    aggregation_enable_value_ = ival;
                    INFO(logger, "Loaded interrupt_coalescing_value: 0x" << std::hex << aggregation_enable_value_);
                } else {
                    WARN(logger, "Invalid interrupt_coalescing_value format, using default: 0x" << std::hex << AGGREGATION_ENABLE);
                }
            } catch (const std::exception &e) {
                WARN(logger, "Failed to parse interrupt_coalescing_value: " << e.what() << ", using default: 0x" << std::hex << AGGREGATION_ENABLE);
            }
        }
        
        if (node["iops_4k_ratio_threshold"]) {
            double ratio = node["iops_4k_ratio_threshold"].as<double>();
            if (ratio > 0 && ratio <= 1.0) {
                iops_4k_ratio_threshold_ = ratio;
                INFO(logger, "Loaded iops_4k_ratio_threshold: " << iops_4k_ratio_threshold_);
            } else {
                WARN(logger, "Invalid iops_4k_ratio_threshold (must be in (0, 1.0]), using default: " << DEFAULT_4K_RATIO_THRESHOLD);
            }
        }
        
        config_loaded_ = true;
        return true;
    } catch (const YAML::Exception &e) {
        WARN(logger, "Failed to parse config file: " << e.what() << ", using defaults.");
        config_loaded_ = true;
        return true;
    }
}

/**
 * @brief 发现系统中的NVMe磁盘
 * 
 * 遍历/sys/block目录，查找所有以"nvme"开头的设备。
 * 
 * @return NVMe磁盘名称列表
 */
std::vector<std::string> DiskAdaptIC::DiscoverNvmeDisks()
{
    std::vector<std::string> nvme_disks;
    
    DIR *dir = opendir(NVME_SYSFS_PATH);
    if (!dir) {
        ERROR(logger, "Failed to open sysfs block directory");
        return nvme_disks;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.compare(0, 4, "nvme") == 0) {
            nvme_disks.push_back(name);
        }
    }
    closedir(dir);
    
    return nvme_disks;
}

/**
 * @brief 判断磁盘是否为NVMe类型
 * 
 * @param disk_name 磁盘名称
 * @return 是NVMe磁盘返回true，否则返回false
 */
bool DiskAdaptIC::IsNvmeDisk(const std::string &disk_name)
{
    return disk_name.compare(0, 4, "nvme") == 0;
}

/**
 * @brief 检查设备节点是否存在
 * 
 * @param dev_path 设备路径
 * @return 设备存在返回true，否则返回false
 */
bool DiskAdaptIC::CheckDeviceExists(const std::string &dev_path)
{
    struct stat st;
    return stat(dev_path.c_str(), &st) == 0;
}

/**
 * @brief 读取磁盘的IO Polling模式状态
 * 
 * 从sysfs读取/queue/io_poll文件，判断磁盘是否处于Polling模式。
 * 
 * @param sysfs_path 磁盘的sysfs路径
 * @return Polling模式开启返回true，否则返回false
 */
bool DiskAdaptIC::ReadIoPollMode(const std::string &sysfs_path)
{
    std::string poll_path = sysfs_path + "/queue/io_poll";
    std::ifstream file(poll_path);
    
    if (!file.is_open()) {
        return false;
    }
    
    int value;
    file >> value;
    return value == 1;
}

/**
 * @brief 读取磁盘统计信息
 * 
 * 从sysfs读取磁盘的IO统计数据，包括读写IO次数和扇区数。
 * 
 * @param sysfs_path 磁盘的sysfs路径
 * @param stats 输出参数，用于存储统计信息
 * @return 读取成功返回true，失败返回false
 */
bool DiskAdaptIC::ReadDiskStats(const std::string &sysfs_path, DiskStats &stats)
{
    std::string stat_path = sysfs_path + "/stat";
    std::ifstream file(stat_path);
    
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    if (!std::getline(file, line)) {
        return false;
    }
    
    std::istringstream iss(line);
    
    // /sys/block/nvmeXnY/stat 字段顺序（标准格式）：
    // 1. read_ios        - 读请求数
    // 2. read_merges     - 合并的读请求数（跳过）
    // 3. read_sectors    - 读扇区数
    // 4. read_ticks      - 读操作耗时(ms)（跳过）
    // 5. write_ios       - 写请求数
    // 6. write_merges    - 合并的写请求数（跳过）
    // 7. write_sectors   - 写扇区数
    // 8. write_ticks     - 写操作耗时(ms)（跳过）
    uint64_t read_merges, read_ticks, write_merges, write_ticks;
    iss >> stats.read_ios >> read_merges >> stats.read_sectors >> read_ticks
        >> stats.write_ios >> write_merges >> stats.write_sectors >> write_ticks;
    
    // 打印读取到的原始数据用于调试
    DEBUG(logger, " ReadDiskStats from " << stat_path);
    DEBUG(logger, "   read_ios: " << stats.read_ios 
         << ", read_merges: " << read_merges 
         << ", read_sectors: " << stats.read_sectors 
         << ", read_ticks: " << read_ticks);
    DEBUG(logger, "   write_ios: " << stats.write_ios 
         << ", write_merges: " << write_merges 
         << ", write_sectors: " << stats.write_sectors 
         << ", write_ticks: " << write_ticks);
    
    stats.ios = stats.read_ios + stats.write_ios;
    stats.bytes = (stats.read_sectors + stats.write_sectors) * 512;
    
    // 使用 std::chrono 获取毫秒级时间戳
    stats.timestamp = GetCurrentTimeMs();
    
    return true;
}

/**
 * @brief 计算IOPS值
 * 
 * 根据两次采样的统计数据计算瞬时IOPS。
 * 
 * @param current 当前采样的统计数据
 * @param prev 上一次采样的统计数据
 * @param time_diff_ms 两次采样的时间间隔（毫秒）
 * @return 计算得到的IOPS值
 */
double DiskAdaptIC::CalculateIops(const DiskStats &current, const DiskStats &prev, uint64_t time_diff_ms)
{
    DEBUG(logger, " CalculateIops - time_diff_ms: " << time_diff_ms 
         << ", current.ios: " << current.ios << ", prev.ios: " << prev.ios);
    
    if (time_diff_ms == 0) {
        DEBUG(logger, " CalculateIops - time_diff_ms is 0, returning 0.0");
        return 0.0;
    }
    
    uint64_t io_diff = current.ios - prev.ios;
    double iops = (io_diff * 1000.0) / time_diff_ms;
    DEBUG(logger, " CalculateIops - io_diff: " << io_diff << ", calculated IOPS: " << std::fixed << std::setprecision(2) << iops);
    return iops;
}

/**
 * @brief 计算平均IO块大小
 * 
 * 根据两次采样的统计数据计算平均IO块大小。
 * 
 * @param current 当前采样的统计数据
 * @param prev 上一次采样的统计数据
 * @return 平均块大小（字节）
 */
uint64_t DiskAdaptIC::CalculateAvgBlockSize(const DiskStats &current, const DiskStats &prev)
{
    uint64_t io_diff = current.ios - prev.ios;
    uint64_t byte_diff = current.bytes - prev.bytes;
    
    if (io_diff == 0 || byte_diff == 0) {
        return 0;
    }
    
    return byte_diff / io_diff;
}

/**
 * @brief 检查4K块占比是否超过阈值
 * 
 * 通过平均块大小近似判断4K块占比是否超过阈值。
 * 当平均块大小 <= 4096 * 2 时认为4K块占比超过阈值。
 * 
 * @param avg_block_size_bytes 平均块大小（字节）
 * @return 4K块占比超过阈值返回true，否则返回false
 */
bool DiskAdaptIC::Check4KBlockRatio(double avg_block_size_bytes)
{
    if (avg_block_size_bytes == 0) {
        return false;
    }
    // 根据4K块占比阈值计算平均块大小阈值：平均块大小 = 4K / 4K块占比
    int block_size_threshold = static_cast<int>(4096.0 / iops_4k_ratio_threshold_);
    DEBUG(logger, " Check4KBlockRatio - avg_block_size: " << avg_block_size_bytes 
         << ", threshold: " << block_size_threshold << ", ratio: " << iops_4k_ratio_threshold_);
    return avg_block_size_bytes <= block_size_threshold;
}

/**
 * @brief 设置中断聚合参数
 * 
 * 调用nvme-cli工具设置NVMe磁盘的中断聚合参数。
 * 
 * @param dev_path 设备路径（如/dev/nvme0n1）
 * @param value 聚合参数值（0x0关闭，0x102开启）
 * @return 设置成功返回true，失败返回false
 */
bool DiskAdaptIC::SetInterruptCoalescing(const std::string &dev_path, int value)
{
    // 先获取当前值，如果已经是目标值就跳过设置
    int current_value = 0;
    if (GetInterruptCoalescing(dev_path, current_value)) {
        if (current_value == value) {
            DEBUG(logger, "SetInterruptCoalescing: Already at target value (0x" << std::hex << value << "), skipping");
            return true;  // 已经是目标值，无需操作
        }
        INFO(logger, "SetInterruptCoalescing: Current (0x" << std::hex << current_value << ") -> Target (0x" << value << ")");
    } else {
        WARN(logger, "SetInterruptCoalescing: Failed to get current value, proceeding with set");
    }
    
    // 将命名空间路径 /dev/nvme0n1 转换为控制器路径 /dev/nvme0
    std::string ctrl_path = dev_path;
    size_t pos = ctrl_path.rfind("n");
    if (pos != std::string::npos && pos > 4) {  // 确保在 "nvme" 之后
        ctrl_path = ctrl_path.substr(0, pos);
    }
    
    std::string cmd = "nvme set-feature " + ctrl_path + " --feature-id=8 --value=" + std::to_string(value) + " 2>/dev/null";
    INFO(logger, "SetInterruptCoalescing: Executing command: " << cmd);
    int ret = system(cmd.c_str());
    bool success = WIFEXITED(ret) && WEXITSTATUS(ret) == 0;
    if (success) {
        INFO(logger, "SetInterruptCoalescing: Command executed successfully");
    } else {
        if (ret == -1) {
            ERROR(logger, "SetInterruptCoalescing: Failed to execute command (system call failed)");
        } else if (WIFEXITED(ret)) {
            ERROR(logger, "SetInterruptCoalescing: Command failed with exit code: " << WEXITSTATUS(ret));
        } else if (WIFSIGNALED(ret)) {
            ERROR(logger, "SetInterruptCoalescing: Command killed by signal: " << WTERMSIG(ret));
        } else {
            ERROR(logger, "SetInterruptCoalescing: Command failed with unknown status");
        }
    }
    return success;
}

/**
 * @brief 获取当前中断聚合参数值
 * 
 * 调用nvme-cli工具获取NVMe磁盘当前的中断聚合参数。
 * 
 * @param dev_path 设备路径（如/dev/nvme0n1）
 * @param value 输出参数，用于存储聚合参数值
 * @return 获取成功返回true，失败返回false
 */
bool DiskAdaptIC::GetInterruptCoalescing(const std::string &dev_path, int &value)
{
    // 将命名空间路径 /dev/nvme0n1 转换为控制器路径 /dev/nvme0
    std::string ctrl_path = dev_path;
    size_t pos = ctrl_path.rfind("n");
    if (pos != std::string::npos && pos > 4) {  // 确保在 "nvme" 之后
        ctrl_path = ctrl_path.substr(0, pos);
    }
    
    // 正确提取 Current value 后面的十六进制值，而不是 feature-id
    std::string cmd = "nvme get-feature " + ctrl_path + " --feature-id=8 2>&1 | grep -o 'Current value:0\\?x\\?[0-9a-fA-F]*' | sed 's/Current value://'";
    DEBUG(logger, " GetInterruptCoalescing - Device: " << dev_path 
         << ", Controller: " << ctrl_path 
         << ", Command: " << cmd);
    
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        ERROR(logger, " GetInterruptCoalescing - Failed to open pipe for command");
        return false;
    }
    
    char buffer[32] = {0};
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        // 移除换行符和空白字符
        buffer[strcspn(buffer, "\n\r")] = 0;
        
        // 检查是否读取到有效内容
        if (strlen(buffer) == 0) {
            pclose(pipe);
            DEBUG(logger, " GetInterruptCoalescing - Read empty string");
            return false;
        }
        
        // 解析十六进制值，0是有效值（表示IC关闭）
        char *endptr = nullptr;
        value = strtoul(buffer, &endptr, 16);
        
        // 验证解析是否成功（即使value为0也是有效的）
        if (endptr == buffer) {
            pclose(pipe);
            WARN(logger, " GetInterruptCoalescing - Failed to parse value from: '" << buffer << "'");
            return false;
        }
        
        pclose(pipe);
        DEBUG(logger, " GetInterruptCoalescing - Success! Read value: 0x" << std::hex << value 
             << " (decimal: " << std::dec << value << "), raw: '" << buffer << "'");
        return true;
    }
    
    pclose(pipe);
    ERROR(logger, " GetInterruptCoalescing - Failed to read from pipe or no output");
    return false;
}

/**
 * @brief 将磁盘添加到监控列表
 * 
 * 创建磁盘信息结构，初始化统计数据，并添加到监控列表。
 * 
 * @param disk_name 磁盘名称
 */
void DiskAdaptIC::AddDiskToMonitor(const std::string &disk_name)
{
    if (monitored_disks_.count(disk_name)) {
        return;
    }
    
    DiskInfo info;
    info.name = disk_name;
    info.dev_path = std::string(NVME_DEV_PATH) + "/" + disk_name;
    info.sysfs_path = std::string(NVME_SYSFS_PATH) + "/" + disk_name;
    info.is_nvme = IsNvmeDisk(disk_name);
    
    if (info.is_nvme && CheckDeviceExists(info.dev_path)) {
        info.in_polling_mode = ReadIoPollMode(info.sysfs_path);
        ReadDiskStats(info.sysfs_path, info.current_stats);
        info.prev_stats = info.current_stats;
        monitored_disks_[disk_name] = info;
        INFO(logger, "Added disk to monitor: " << disk_name);
    }
}

/**
 * @brief 从监控列表中移除磁盘
 * 
 * 将指定磁盘从监控列表中删除。
 * 
 * @param disk_name 磁盘名称
 */
void DiskAdaptIC::RemoveDiskFromMonitor(const std::string &disk_name)
{
    monitored_disks_.erase(disk_name);
    INFO(logger, "Removed disk from monitor: " << disk_name);
}

/**
 * @brief 处理设备错误
 * 
 * 当设备发生错误时，增加错误计数并在达到阈值时触发熔断。
 * 
 * @param disk 磁盘信息结构
 */
void DiskAdaptIC::HandleDeviceError(DiskInfo &disk)
{
    if (disk.error_count < 100) {
        disk.error_count++;
    }
    
    if (disk.error_count >= MAX_RETRY_COUNT && !disk.is_fused) {
        disk.last_fuse_time = GetCurrentTimeMs();
        disk.is_fused = true;
        WARN(logger, "Disk " << disk.name << " fused due to errors.");
    }
}

/**
 * @brief 处理设备恢复
 * 
 * 当设备熔断后经过一定时间，自动恢复监控。
 * 
 * @param disk 磁盘信息结构
 */
void DiskAdaptIC::HandleDeviceRecovery(DiskInfo &disk)
{
    uint64_t now = GetCurrentTimeMs();
    
    if (disk.is_fused && (now - disk.last_fuse_time) > FUSE_DELAY_MS) {
        disk.is_fused = false;
        disk.error_count = 0;
        INFO(logger, "Disk " << disk.name << " recovered from fuse.");
    }
}

/**
 * @brief 更新磁盘状态
 * 
 * 检查设备是否存在，读取最新统计数据，计算IOPS和平均块大小，
 * 根据条件判断是否需要开启或关闭中断聚合。
 * 
 * @param disk 磁盘信息结构
 */
void DiskAdaptIC::UpdateDiskStatus(DiskInfo &disk)
{
    if (!CheckDeviceExists(disk.dev_path)) {
        INFO(logger, "Disk " << disk.name << " no longer exists.");
        disk.to_be_removed = true;
        return;
    }
    
    HandleDeviceRecovery(disk);
    
    if (disk.is_fused) {
        return;
    }
    
    DiskStats new_stats;
    if (!ReadDiskStats(disk.sysfs_path, new_stats)) {
        HandleDeviceError(disk);
        return;
    }
    
    disk.prev_stats = disk.current_stats;
    disk.current_stats = new_stats;
    
    uint64_t time_diff = new_stats.timestamp - disk.prev_stats.timestamp;
    double iops = CalculateIops(new_stats, disk.prev_stats, time_diff);
    uint64_t avg_block_size = CalculateAvgBlockSize(new_stats, disk.prev_stats);
    bool is_4k_dominant = Check4KBlockRatio(avg_block_size);
    
    // 输出统计信息
    DEBUG(logger, "Disk[" << disk.name << "] IOPS: " << std::fixed << std::setprecision(2) << iops 
         << ", AvgBlockSize: " << avg_block_size << " bytes (" 
         << (avg_block_size >= 1024 ? std::to_string(avg_block_size / 1024) + " KB" : std::to_string(avg_block_size) + " B") 
         << "), 4K_Dominant: " << (is_4k_dominant ? "Yes" : "No")
         << ", IC_Enabled: " << (disk.ic_enabled ? "Yes" : "No"));
    
    bool should_enable_ic = (iops > iops_threshold_) && is_4k_dominant;
    
    if (should_enable_ic) {
        if (disk.consecutive_high_load < 1) {
            INFO(logger, "Enabling IC on " << disk.name << " - IOPS: " << std::fixed << std::setprecision(2) << iops << ", 4K: " << is_4k_dominant);
            disk.consecutive_high_load++;
        }
        disk.consecutive_low_load = 0;
        
        if (disk.consecutive_high_load >= 1 && !disk.ic_enabled) {
            if (SetInterruptCoalescing(disk.dev_path, aggregation_enable_value_)) {  // 使用配置值
                disk.ic_enabled = true;
                disk.error_count = 0;
            } else {
                HandleDeviceError(disk);
            }
        }
    } else {
        if (disk.consecutive_low_load < 1) {
            INFO(logger, "Disabling IC on " << disk.name << " - IOPS: " << std::fixed << std::setprecision(2) << iops << ", 4K: " << is_4k_dominant);
            disk.consecutive_low_load++;
        }
        disk.consecutive_high_load = 0;
        
        if (disk.consecutive_low_load >= 1 && disk.ic_enabled) {
            if (SetInterruptCoalescing(disk.dev_path, AGGREGATION_DISABLE)) {
                disk.ic_enabled = false;
                disk.error_count = 0;
            } else {
                HandleDeviceError(disk);
            }
        }
    }
}

/**
 * @brief 打开Topic（接口实现）
 * 
 * @param topic Topic信息
 * @return 操作结果
 */
oeaware::Result DiskAdaptIC::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

/**
 * @brief 关闭Topic（接口实现）
 * 
 * @param topic Topic信息
 */
void DiskAdaptIC::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

/**
 * @brief 更新数据（接口实现）
 * 
 * @param dataList 数据列表
 */
void DiskAdaptIC::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

/**
 * @brief 检查polling模式是否启用
 * 
 * 检查所有监控磁盘的polling模式，如果任一磁盘启用了polling则返回true。
 * 同时更新每个磁盘的in_polling_mode状态。
 * 
 * @return 任一磁盘启用了polling模式返回true，否则返回false
 */
bool DiskAdaptIC::CheckPollingMode()
{
    bool polling_enabled = false;
    
    for (auto &entry : monitored_disks_) {
        DiskInfo &disk = entry.second;
        std::string polling_path = disk.sysfs_path + "/queue/io_poll";
        std::ifstream file(polling_path);
        
        if (file.good()) {
            int value = 0;
            file >> value;
            disk.in_polling_mode = (value == 1);
            
            if (disk.in_polling_mode) {
                polling_enabled = true;
                WARN(logger, "Disk " << disk.name << " has IO polling mode enabled, interrupt coalescing may have limited effect.");
            } else {
                DEBUG(logger, "Disk " << disk.name << " is in interrupt mode.");
            }
        } else {
            // 文件不存在可能是磁盘类型不支持此功能，不视为错误
            DEBUG(logger, "Cannot read polling mode for disk " << disk.name << ", file not found.");
            disk.in_polling_mode = false;
        }
    }
    
    return polling_enabled;
}

/**
 * @brief 检查nvme命令包是否安装
 * 
 * 通过执行nvme命令检查是否安装了nvme-cli工具。
 * 
 * @return 安装了返回true，否则返回false
 */
bool DiskAdaptIC::CheckNvmeCliInstalled()
{
    int ret = system("which nvme > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        return true;
    }
    return false;
}



/**
 * @brief 使能插件
 * 
 * 加载配置，检查环境，发现NVMe磁盘，启动监控线程。
 * 
 * @param param 参数（未使用）
 * @return 操作结果
 */
oeaware::Result DiskAdaptIC::Enable(const std::string &param)
{
    (void)param;
    
    // 恢复所有配置参数为默认值
    iops_threshold_ = DEFAULT_IOPS_THRESHOLD;
    sample_period_ms_ = DEFAULT_SAMPLE_PERIOD_MS;
    iops_4k_ratio_threshold_ = DEFAULT_4K_RATIO_THRESHOLD;
    aggregation_enable_value_ = AGGREGATION_ENABLE;
    config_loaded_ = false;
    
    // 检查nvme-cli是否安装（必须先检查，因为后续操作需要）
    if (!CheckNvmeCliInstalled()) {
        return oeaware::Result(FAILED, "nvme-cli is not installed, please install it first.");
    }
    INFO(logger, "nvme-cli is installed");
    
    // 读取配置文件
    if (!LoadConfig()) {
        WARN(logger, "Failed to load config, using defaults.");
    }
    
    // 发现并添加监控磁盘（必须在检查polling模式之前完成）
    std::vector<std::string> nvme_disks = DiscoverNvmeDisks();
    if (nvme_disks.empty()) {
        return oeaware::Result(FAILED, "No NVMe disks found.");
    }
    
    for (const auto &disk_name : nvme_disks) {
        AddDiskToMonitor(disk_name);
    }
    
    // 对每个磁盘检查polling模式（需要先有磁盘列表）
    // 如果有任一磁盘启用了polling模式，则不启用插件
    if (CheckPollingMode()) {
        monitored_disks_.clear();  // 清理已添加的磁盘
        return oeaware::Result(FAILED, "Cannot enable plugin because some disks are in IO polling mode.");
    }
    
    // 先关闭所有磁盘的中断聚合，初始状态从干净状态开始
    for (auto &entry : monitored_disks_) {
            SetInterruptCoalescing(entry.second.dev_path, AGGREGATION_DISABLE);
    }
    
    // 设置启用标志，Run() 方法会检查此标志
    is_enabled_ = true;
    
    INFO(logger, "Disk adaptive IC plugin enabled, monitoring " << nvme_disks.size() << " disk(s).");
    return oeaware::Result(OK);
}

/**
 * @brief 关闭插件
 * 
 * 清理监控列表，关闭所有磁盘的中断聚合。
 */
void DiskAdaptIC::Disable()
{
    DEBUG(logger, " Disable() - Entering Disable method");
    
    // 清除启用标志，Run() 方法会检查此标志
    is_enabled_ = false;
    DEBUG(logger, " Disable() - is_enabled_ set to false");
    
    DEBUG(logger, " Disable() - Iterating over monitored_disks_, count: " << monitored_disks_.size());
    for (auto &entry : monitored_disks_) {
        DEBUG(logger, " Disable() - Processing disk: " << entry.first << ", ic_enabled: " << entry.second.ic_enabled);
        if (entry.second.ic_enabled) {
            DEBUG(logger, " Disable() - Calling SetInterruptCoalescing for: " << entry.second.dev_path);
            SetInterruptCoalescing(entry.second.dev_path, AGGREGATION_DISABLE);
            DEBUG(logger, " Disable() - SetInterruptCoalescing returned for: " << entry.second.dev_path);
        }
    }
    
    DEBUG(logger, " Disable() - Clearing monitored_disks_");
    monitored_disks_.clear();
    
    DEBUG(logger, " Disable() - Exiting Disable method");
    INFO(logger, "Disk adaptive IC plugin disabled.");
}

/**
 * @brief 运行方法（接口实现）
 * 
 * 直接在Run中实现统计和计算，框架会定期调用此方法（约100ms一次）。
 * 检查新磁盘、更新磁盘状态、根据IOPS负载动态调整中断聚合参数。
 */
void DiskAdaptIC::Run()
{
    DEBUG(logger, " Run() - Entering Run method, is_enabled_: " << is_enabled_);
    
    if (!is_enabled_) {
        DEBUG(logger, " Run() - Plugin not enabled, returning");
        return;
    }
    
    // 检查配置的采样间隔
    uint64_t now = GetCurrentTimeMs();
    uint64_t time_since_last_run = now - last_run_time_ms_;
    
    DEBUG(logger, " Run() - now: " << now 
         << ", last_run_time_ms_: " << last_run_time_ms_ 
         << ", time_since_last_run: " << time_since_last_run 
         << ", sample_period_ms_: " << sample_period_ms_);
    
    if (time_since_last_run < sample_period_ms_) {
        DEBUG(logger, " Run() - Skip, time since last run (" << time_since_last_run 
             << "ms) < sample period (" << sample_period_ms_ << "ms)");
        return;
    }
    last_run_time_ms_ = now;
    
    try {
        // 1. 发现新磁盘
        DEBUG(logger, " Run() - Step 1: Discovering NVMe disks");
        std::vector<std::string> current_disks = DiscoverNvmeDisks();
        DEBUG(logger, " Run() - Found " << current_disks.size() << " disks");
        
        for (const auto &disk_name : current_disks) {
            if (!monitored_disks_.count(disk_name)) {
                DEBUG(logger, " Run() - Adding " << disk_name << " to monitor");
                AddDiskToMonitor(disk_name);
            }
        }
        
        // 2. 更新所有监控磁盘的状态
        DEBUG(logger, " Run() - Step 2: Updating disk status, monitored count: " << monitored_disks_.size());
        for (auto &entry : monitored_disks_) {
            if (entry.second.is_enabled) {
                DEBUG(logger, " Run() - Updating status for: " << entry.first);
                UpdateDiskStatus(entry.second);
            }
        }
        
        // 3. 统一清理标记为待删除的磁盘（避免迭代器失效）
        DEBUG(logger, " Run() - Step 3: Cleaning up removed disks");
        std::vector<std::string> to_remove;
        for (const auto &entry : monitored_disks_) {
            if (entry.second.to_be_removed) {
                to_remove.push_back(entry.first);
            }
        }
        for (const auto &disk_name : to_remove) {
            RemoveDiskFromMonitor(disk_name);
        }
        
        DEBUG(logger, " Run() - Exiting Run method");
        
    } catch (const std::exception &e) {
        ERROR(logger, "[DEBUG] Run() - Exception caught: " << e.what());
    } catch (...) {
        ERROR(logger, "[DEBUG] Run() - Unknown exception caught");
    }
}

#include <iostream>
#include <vector>
#include <cstdio>
#include <chrono>
#include <securec.h>
#include <fstream>
#include <sstream>
/*
* g++ read_proc_stat.cpp  -o read_proc_stat -lboundscheck
* g++ read_proc_stat.cpp  -o read_proc_stat -lboundscheck -O2
* ./read_proc_stat 4 1000
*/
using EnvCpuUtilType = enum {
    CPU_USER,
    CPU_NICE,
    CPU_SYSTEM,
    CPU_IDLE,
    CPU_IOWAIT,
    CPU_IRQ,
    CPU_SOFTIRQ,
    CPU_STEAL,
    CPU_GUEST,
    CPU_GNICE,
    CPU_TIME_SUM,  // sum of all above
    CPU_UTIL_TYPE_MAX,
};

// 生成随机 CPU 时间数据
bool UpdateProcStat(const std::string &statpath, int cpuNum, std::vector<std::vector<uint64_t>> &cpuTime)
{
    std::string path = statpath;
    FILE *file = fopen(path.c_str(), "r");
    if (!file) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return false;
    }
    if (fscanf_s(file, "%*s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
        &cpuTime[cpuNum][CPU_USER],
        &cpuTime[cpuNum][CPU_NICE],
        &cpuTime[cpuNum][CPU_SYSTEM],
        &cpuTime[cpuNum][CPU_IDLE],
        &cpuTime[cpuNum][CPU_IOWAIT],
        &cpuTime[cpuNum][CPU_IRQ],
        &cpuTime[cpuNum][CPU_SOFTIRQ],
        &cpuTime[cpuNum][CPU_STEAL],
        &cpuTime[cpuNum][CPU_GUEST],
        &cpuTime[cpuNum][CPU_GNICE]) != CPU_UTIL_TYPE_MAX - 1) {
        if (fclose(file) == EOF) {
            std::cerr << "Failed to close file: " << path << std::endl;
        }
        return false;
    }

    while (cpuNum--) {
        int readCpu = -1; // adapt offline cpu
        if (fscanf_s(file, "cpu%d ", &readCpu) != 1) {
            break; // read finish
        }
        if (fscanf_s(file, " %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
            &cpuTime[readCpu][CPU_USER],
            &cpuTime[readCpu][CPU_NICE],
            &cpuTime[readCpu][CPU_SYSTEM],
            &cpuTime[readCpu][CPU_IDLE],
            &cpuTime[readCpu][CPU_IOWAIT],
            &cpuTime[readCpu][CPU_IRQ],
            &cpuTime[readCpu][CPU_SOFTIRQ],
            &cpuTime[readCpu][CPU_STEAL],
            &cpuTime[readCpu][CPU_GUEST],
            &cpuTime[readCpu][CPU_GNICE]) != CPU_UTIL_TYPE_MAX - 1) {
            if (fclose(file) == EOF) {
                std::cerr << "Failed to close file: " << path << std::endl;
            }
            return false;
        }
    }

    if (fclose(file) == EOF) {
        std::cerr << "Failed to close file: " << path << std::endl;
    }
    return true;
}

bool UpdateProcStat2(const std::string &statpath, int cpuNum, std::vector<std::vector<uint64_t>> &cpuTime) {
    const std::string path = statpath;
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    // 一次性读取整个文件内容
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // 解析总 CPU 数据
    std::istringstream iss(content);
    std::string line;
    if (!std::getline(iss, line)) {
        std::cerr << "Failed to read total CPU data." << std::endl;
        return false;
    }

    std::istringstream totalCpuStream(line);
    std::string label;
    totalCpuStream >> label; // 读取 "cpu" 标签

    for (int i = 0; i < CPU_UTIL_TYPE_MAX - 1; ++i) {
        if (!(totalCpuStream >> cpuTime[cpuNum][i])) {
            std::cerr << "Failed to parse total CPU data." << std::endl;
            return false;
        }
    }

    // 解析每个逻辑 CPU 的数据
    while (std::getline(iss, line)) {
        if (line.compare(0, 3, "cpu") != 0 || !isdigit(line[3])) {
            break; // 非 CPU 数据行，退出循环
        }

        std::istringstream cpuStream(line);
        std::string cpuLabel;
        int readCpu = -1;

        cpuStream >> cpuLabel; // 读取 "cpuX" 标签
        if (sscanf_s(cpuLabel.c_str(), "cpu%d", &readCpu) != 1 || readCpu < 0 || readCpu >= cpuNum) {
            continue; // 跳过无效的 CPU 数据
        }

        for (int i = 0; i < CPU_UTIL_TYPE_MAX - 1; ++i) {
            if (!(cpuStream >> cpuTime[readCpu][i])) {
                std::cerr << "Failed to parse CPU " << readCpu << " data." << std::endl;
                return false;
            }
        }
    }

    return true;
}
int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <cpuNum> <times>" << std::endl;
        return -1;
    }
    int cpuNum = atoi(argv[1]);
    int times = atoi(argv[2]);
    const std::string statpath = "/proc/stat";
    std::vector<std::vector<uint64_t>> cpuTime; // [cpu][type]
    cpuTime.resize(cpuNum + 1, std::vector<uint64_t>(CPU_UTIL_TYPE_MAX, 0));
    auto t1 = std::chrono::system_clock::now();
    for (int i = 0; i < times; i++) {
        UpdateProcStat(statpath, cpuNum, cpuTime);
    }
    auto t2 = std::chrono::system_clock::now();
    std::cout << "UpdateProcStat time cost: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "ms" << std::endl;
    for (int i = 0; i < times; i++) {
        UpdateProcStat2(statpath, cpuNum, cpuTime);
    }
    auto t3 = std::chrono::system_clock::now();
    std::cout << "UpdateProcStat2 time cost: " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << "ms" << std::endl;
    return 0;
}
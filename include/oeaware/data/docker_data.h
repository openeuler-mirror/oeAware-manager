
#ifndef OEAWARE_DATA_DOCKER_COLLECTOR_H
#define OEAWARE_DATA_DOCKER_COLLECTOR_H
#include <cstdint>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
struct Container {
    std::string id;   // change to char* if need to serialization
    int64_t cfs_period_us;
    int64_t cfs_quota_us;
    int64_t cfs_burst_us;
    int64_t cpu_usage; // Total CPU Time (cpuacct.usage)
    int64_t sampling_timestamp;
    std::string cpus; // change to char* if need to serialization
    std::vector<int32_t> tasks; // change to int32_t* if need to serialization
};

#ifdef __cplusplus
}
#endif

#endif
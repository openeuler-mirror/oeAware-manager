
#ifndef OEAWARE_DATA_DOCKER_COLLECTOR_H
#define OEAWARE_DATA_DOCKER_COLLECTOR_H
#include <cstdint>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
struct Container {
    std::string id;
    int64_t cfs_period_us;
    int64_t cfs_quota_us;
    int64_t cfs_burst_us;
};

#ifdef __cplusplus
}
#endif

#endif
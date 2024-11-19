
#ifndef __DOCKER_COLLECTOR_H__
#define __DOCKER_COLLECTOR_H__
#include <cstdint>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#define DOCKER_COLLECTOR "docker_collector"

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

#include <iostream>
#include <string>
#include <unistd.h>
#include <iomanip>
#include "oeaware/data/env_data.h"
#include "oeaware/data_list.h"
#include "oe_client.h"

static int ShowEnvStaticInfo(const DataList *dataList)
{
    static bool finish = false;
    if (finish) {
        return 0;
    }
    if (dataList && dataList->len && dataList->data) {
        EnvStaticInfo *data = static_cast<EnvStaticInfo *>(dataList->data[0]);
        std::cout << "cpuNumConfig: " << data->cpuNumConfig << std::endl;
        std::cout << "numaNum: " << data->numaNum << std::endl;
        std::cout << "pageSize:" << data->pageSize << std::endl;
        std::cout << "pageMask: 0x" << std::hex << data->pageMask << std::endl;
        for (int cpu = 0; cpu < data->cpuNumConfig; cpu++) {
            std::cout << std::dec << "cpu " << cpu << ", numa " << data->cpu2Node[cpu] << std::endl;
        }
        std::cout << "numaDistance:" << std::endl;
        for (int ni = 0; ni < data->numaNum; ni++) {
            for (int nj = 0; nj < data->numaNum; nj++) {
                std::cout << data->numaDistance[ni][nj] << " ";
            }
            std::cout << std::endl;
        }
        finish = true;
    }
    return 0;
}

static int ShowEnvRealTimeInfo(const DataList *dataList)
{
    if (dataList && dataList->len && dataList->data) {
        EnvRealTimeInfo *data = static_cast<EnvRealTimeInfo *>(dataList->data[0]);
        std::cout << "dataReady: " << data->dataReady << std::endl;
        std::cout << "cpuNumConfig: " << data->cpuNumConfig << std::endl;
        std::cout << "cpuNumOnline: " << data->cpuNumOnline << std::endl;
        for (int i = 0; i < data->cpuNumConfig; i++) {
            std::cout << "cpu " << i << ", online " << data->cpuOnLine[i] << std::endl;
        }
    }
    return 0;
}

static int ShowCpuUtilInfo(const DataList *dataList)
{
    if (dataList && dataList->len && dataList->data) {
        EnvCpuUtilParam *data = static_cast<EnvCpuUtilParam *>(dataList->data[0]);
        std::cout << "dataReady: " << data->dataReady << std::endl;
        std::cout << "cpuNumConfig: " << data->cpuNumConfig << std::endl;
        for (int cpu = 0; cpu < data->cpuNumConfig && cpu < 10; cpu++) {
            std::cout << "cpu " << cpu << " ";
            for (int type = 0; type < CPU_UTIL_TYPE_MAX; type++) {
                std::cout << std::fixed << std::setprecision(1) << std::setw(5) \
                    << data->times[cpu][type] * 100.0 / data->times[cpu][CPU_TIME_SUM] << "% ";
            }
            std::cout << std::endl;
        }
    }
    return 0;
}

bool TestEnvStaticInfo()
{
    CTopic staticEnvInfo = { OE_ENV_INFO, "static", "" };
    int ret = OeInit();
    if (ret != 0) {
        std::cout << " SDK(Analysis) Init failed , result " << ret << std::endl;
        return false;
    }

    OeSubscribe(&staticEnvInfo, ShowEnvStaticInfo);
    sleep(2);
    OeUnsubscribe(&staticEnvInfo);
    OeClose();
    return true;
}

bool TestEnvRealTimeInfo()
{
    CTopic realTimeEnvInfo = { OE_ENV_INFO, "realtime", "" };
    int ret = OeInit();
    if (ret != 0) {
        std::cout << " SDK(Analysis) Init failed , result " << ret << std::endl;
        return false;
    }

    OeSubscribe(&realTimeEnvInfo, ShowEnvRealTimeInfo);
    sleep(2);
    OeUnsubscribe(&realTimeEnvInfo);
    OeClose();
    return true;
}

bool TestCpuUtilInfo()
{
    CTopic topic = { OE_ENV_INFO, "cpu_util", "" };
    int ret = OeInit();
    if (ret != 0) {
        std::cout << " SDK(Analysis) Init failed , result " << ret << std::endl;
        return false;
    }

    OeSubscribe(&topic, ShowCpuUtilInfo);
    sleep(3);
    OeUnsubscribe(&topic);
    OeClose();
    return true;
}
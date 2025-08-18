#include <iostream>
#include <string>
#include <unistd.h>
#include <iomanip>
#include "oeaware/data_list.h"
#include "oeaware/data/pmu_uncore_data.h"
#include "oe_client.h"

static int ShowPmuUncoreInfo(const DataList *dataList)
{
    if (dataList && dataList->len && dataList->data) {
        PmuUncoreData *data = static_cast<PmuUncoreData *>(dataList->data[0]);
        std::cout << "interval: " << data->interval << ", len: " << data->len << std::endl;
        for (int i = 0; i < data->len; i++) {
            std::cout << "evt: " << data->pmuData[i].evt << ", count: " << data->pmuData[i].count << std::endl;
        }
    }
    return 0;
}


bool TestPmuUncoreInfo(int time)
{
    CTopic topic = { OE_PMU_UNCORE_COLLECTOR, "uncore", "" };
    int ret = OeInit();
    if (ret != 0) {
        std::cout << " SDK(Analysis) Init failed , result " << ret << std::endl;
        return false;
    }

    OeSubscribe(&topic, ShowPmuUncoreInfo);
    sleep(time);
    OeUnsubscribe(&topic);
    OeClose();
    return true;
}
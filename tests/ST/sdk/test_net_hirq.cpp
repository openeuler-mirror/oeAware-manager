
#include <iostream>
#include <string>
#include <unistd.h>
#include <iomanip>
#include "oeaware/data_list.h"
#include "oeaware/data/net_hardirq_tune_data.h"
#include "oe_client.h"

static int ShowHirqTuneDebugLog(const DataList *dataList)
{
    if (dataList && dataList->len && dataList->data) {
        NetHirqTuneDebugInfo *data = static_cast<NetHirqTuneDebugInfo *>(dataList->data[0]);
        std::cout << data->log << std::endl;
    }
    return 0;
}


bool TestHirqTuneDebug(int time)
{
    CTopic topic = { OE_NETHARDIRQ_TUNE, OE_TOPIC_NET_HIRQ_TUNE_DEBUG_INFO, "" };
    int ret = OeInit();
    if (ret != 0) {
        std::cout << " SDK(Analysis) Init failed , result " << ret << std::endl;
        return false;
    }

    OeSubscribe(&topic, ShowHirqTuneDebugLog);
    sleep(time);
    OeUnsubscribe(&topic);
    OeClose();
    return true;
}

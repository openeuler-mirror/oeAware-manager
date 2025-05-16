
#include <iostream>
#include <string>
#include <unistd.h>
#include <iomanip>
#include "oeaware/data/network_interface_data.h"
#include "oeaware/data_list.h"
#include "oe_client.h"
#include "oeaware/utils.h"
static int ShowNetIntfBaseInfo(const DataList *dataList)
{
    if (dataList && dataList->len && dataList->data) {
        std::string topicParams = std::string(dataList->topic.params);
        NetIntfBaseDataList *data = static_cast<NetIntfBaseDataList *>(dataList->data[0]);
        std::cout << topicParams << " data->count: " << data->count << std::endl;
        for (int n = 0; n < data->count; n++) {
            std::cout << std::string(data->base[n].name) << " "
                << oeaware::GetNetOperateStr(data->base[n].operstate)
                << ", ifindex" << data->base[n].ifindex << std::endl;
        }
    }
    return 0;
}

static int ShowNetIntfDriverInfo(const DataList *dataList)
{
    if (dataList && dataList->len && dataList->data) {
        std::string topicParams = std::string(dataList->topic.params);
        NetIntfDriverDataList *data = static_cast<NetIntfDriverDataList *>(dataList->data[0]);
        std::cout << topicParams << " data->count: " << data->count << std::endl;
        for (int n = 0; n < data->count; n++) {
            std::cout << std::string(data->driver[n].name) << std::endl;
            std::cout << "driver: " << std::string(data->driver[n].driver) << std::endl;
            std::cout << "version: " << std::string(data->driver[n].version) << std::endl;
            std::cout << "bus info: " << std::string(data->driver[n].busInfo) << std::endl;
            std::cout << "firmware version: " << std::string(data->driver[n].fwVersion) << std::endl;
        }
    }
    return 0;
}

static int ShowNetLocalAffiInfo(const DataList *dataList)
{
    if (dataList && dataList->len && dataList->data) {
        std::string topicParams = std::string(dataList->topic.params);
        ProcessNetAffinityDataList *data = static_cast<ProcessNetAffinityDataList *>(dataList->data[0]);
        std::cout << topicParams << " data->count: " << data->count << std::endl;
        for (int n = 0; n < data->count; n++) {
            std::cout << "pid1 " << data->affinity[n].pid1 << ", pid2 " << data->affinity[n].pid2
                << ", level " << data->affinity[n].level << std::endl;
        }
    }
    return 0;
}

bool TestNetIntfBaseInfo(int time)
{
    CTopic topic1 = { OE_NET_INTF_INFO,  OE_NETWORK_INTERFACE_BASE_TOPIC, "operstate_all" };
    CTopic topic2 = { OE_NET_INTF_INFO,  OE_NETWORK_INTERFACE_BASE_TOPIC, "operstate_up" };
    int ret = OeInit();
    if (ret != 0) {
        std::cout << " SDK(Analysis) Init failed , result " << ret << std::endl;
        return false;
    }

    OeSubscribe(&topic1, ShowNetIntfBaseInfo);
    OeSubscribe(&topic2, ShowNetIntfBaseInfo);
    sleep(time);
    OeUnsubscribe(&topic1);
    OeUnsubscribe(&topic2);
    OeClose();
    return true;
}

bool TestNetIntfDirverInfo(int time)
{
    CTopic topic1 = { OE_NET_INTF_INFO,  OE_NETWORK_INTERFACE_DRIVER_TOPIC, "operstate_all" };
    CTopic topic2 = { OE_NET_INTF_INFO,  OE_NETWORK_INTERFACE_DRIVER_TOPIC, "operstate_up" };
    int ret = OeInit();
    if (ret != 0) {
        std::cout << " SDK(Analysis) Init failed , result " << ret << std::endl;
        return false;
    }

    OeSubscribe(&topic1, ShowNetIntfDriverInfo);
    OeSubscribe(&topic2, ShowNetIntfDriverInfo);
    sleep(time);
    OeUnsubscribe(&topic1);
    OeUnsubscribe(&topic2);
    OeClose();
    return true;
}

bool TestNetLocalAffiInfo(int time)
{
    CTopic topic1 = { OE_NET_INTF_INFO,  OE_LOCAL_NET_AFFINITY, OE_PARA_LOC_NET_AFFI_USER_DEBUG };
    CTopic topic2 = { OE_NET_INTF_INFO,  OE_LOCAL_NET_AFFINITY, OE_PARA_PROCESS_AFFINITY };
    
    int ret = OeInit();
    if (ret != 0) {
        std::cout << " SDK(Analysis) Init failed , result " << ret << std::endl;
        return false;
    }

    OeSubscribe(&topic1, ShowNetLocalAffiInfo);
    OeSubscribe(&topic2, ShowNetLocalAffiInfo);
    sleep(time);
    OeUnsubscribe(&topic1);
    OeUnsubscribe(&topic2);
    OeClose();
    return true;
}
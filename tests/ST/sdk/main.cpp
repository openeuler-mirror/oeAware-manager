#include <iostream>
#include <string>
#include <functional>
#include <map>
#include <iomanip>
#include "oeaware/data_list.h"
#include "oeaware/data/network_interface_data.h"
#include "oeaware/data/net_hardirq_tune_data.h"
#include "test_inc.h"

using CallbackFunction = std::function<bool(int)>;

class CallbackManager {
public:
    void RegisterCallback(const std::string &name, CallbackFunction callback, const std::string &description)
    {
        callbacks[name] = callback;
        descriptions[name] = description;
    }

    bool InvokeCallback(const std::string &name, int time)
    {
        auto it = callbacks.find(name);
        if (it != callbacks.end()) {
            return it->second(time);
        } else {
            std::cout << "Callback not found: " << name << std::endl;
            return false;
        }
    }

    bool IsValidOption(const std::string &option) const
    {
        return callbacks.find(option) != callbacks.end();
    }

    void ShowCallbackDescriptions() const
    {
        std::cout << "usage: test_sdk [options] time.." << std::endl;
        std::cout << "options:" << std::endl;
        for (const auto &entry : descriptions) {
            std::cout << "       " << std::left << std::setw(40) <<
                entry.first << "             " << entry.second << std::endl;
        }
    }

private:
    std::map<std::string, CallbackFunction> callbacks;
    std::map<std::string, std::string> descriptions;
};

int main(int argc, char *argv[])
{
    CallbackManager manager;
    std::string options;
    int time = 2; // default time is 2s
    manager.RegisterCallback(std::string(OE_ENV_INFO) + "::static", TestEnvStaticInfo, "show environment static info");
    manager.RegisterCallback(std::string(OE_ENV_INFO) + "::realtime", TestEnvRealTimeInfo, "show environment realtime info");
    manager.RegisterCallback(std::string(OE_ENV_INFO) + "::cpu_util", TestCpuUtilInfo, "show cpu util info");
    manager.RegisterCallback(std::string(OE_NET_INTF_INFO) + std::string("::") + std::string(OE_NETWORK_INTERFACE_BASE_TOPIC), TestNetIntfBaseInfo, "show base net intf info");
    manager.RegisterCallback(std::string(OE_NET_INTF_INFO) + std::string("::") + std::string(OE_NETWORK_INTERFACE_DRIVER_TOPIC), TestNetIntfDirverInfo, "show driver net intf info");
    manager.RegisterCallback(std::string(OE_NET_INTF_INFO) + std::string("::") + std::string(OE_LOCAL_NET_AFFINITY), TestNetLocalAffiInfo, "show local net affinity info");
    manager.RegisterCallback(std::string(OE_NETHARDIRQ_TUNE) + std::string("::") +
        std::string(OE_TOPIC_NET_HIRQ_TUNE_DEBUG_INFO), TestHirqTuneDebug, "show net hirq tune debug info");
    manager.RegisterCallback(std::string(OE_NET_INTF_INFO) + std::string("::") +
        std::string(OE_PARA_THREAD_RECV_QUE_CNT), TestNetThrQueInfo, "show net threads queue and debug info");
    if (argc >= 2) {
        options = std::string(argv[1]);
        if (!manager.IsValidOption(options)) {
            manager.ShowCallbackDescriptions();
            return -1;
        }
        if (argc >= 3) {
            time = std::stoi(argv[2]);
        }
    } else {
        manager.ShowCallbackDescriptions();
        return -1;
    }
    if (!manager.InvokeCallback(options, time)) {
        std::cout << "ST Test failed, Case:" << options << std::endl;
        return -1;
    }
    return 0;
}
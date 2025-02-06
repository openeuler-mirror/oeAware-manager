#include "hardirq_tune.h"

using namespace oeaware;

NetHardIrq::NetHardIrq()
{
    name = OE_NETHARDIRQ_TUNE;
    version = "1.0.0";
    period = 1000;
    priority = 2;
    type = TUNE;
    description += "[introduction] \n";
    description += "               Bind the interrupt corresponding to the network interface queue to NUMA, \n";
    description += "               which receives packets frequently in the network, to accelerate network processing\n";
    description += "[version] \n";
    description += "         " + version + "\n";
    description += "[instance name]\n";
    description += "                 " + name + "\n";
    description += "[instance period]\n";
    description += "                 " + std::to_string(period) + " ms\n";
    description += "[provide topics]\n";
    description += "                 none\n";
    description += "[running require arch]\n";
    description += "                      aarch64 only\n";
    description += "[running require environment]\n";
    description += "                 physical machine(not qemu), kernel version(4.19, 5.10, 6.6)\n";
    description += "[running require topics]\n";
    description += "                        none\n";
    description += "[usage] \n";
    description += "        example:You can use `oeawarectl -e net_hard_irq_tune` to enable\n";

}

oeaware::Result NetHardIrq::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void NetHardIrq::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void NetHardIrq::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

oeaware::Result NetHardIrq::Enable(const std::string &param)
{
    bool isActive;
    irqbalanceStatus = false;
    // irq tune confilct with irqbalance
    if (ServiceIsActive("irqbalance", isActive))
    {
        irqbalanceStatus = isActive;
    }
    if (irqbalanceStatus) {
        ServiceControl("irqbalance", "stop");
    }
    return oeaware::Result(OK);
}

void NetHardIrq::Disable()
{
    if (irqbalanceStatus) {
        ServiceControl("irqbalance", "start");
    }
}

void NetHardIrq::Run()
{
}

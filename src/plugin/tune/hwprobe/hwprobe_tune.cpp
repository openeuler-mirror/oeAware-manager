#include "oeaware/interface.h"
#include "ext/hwprobe_ext_zbb_tune/hwprobe_ext_zbb_tune.h"
using namespace oeaware;

extern "C" void GetInstance(std::vector<std::shared_ptr<oeaware::Interface>> &interface)
{
    interface.emplace_back(std::make_shared<HwprobeExtZbbTune>());
}
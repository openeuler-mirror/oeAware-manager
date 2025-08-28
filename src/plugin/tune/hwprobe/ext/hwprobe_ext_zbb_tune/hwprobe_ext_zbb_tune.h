// hwprobe_ext_zbb_tune.h
#ifndef HWPROBE_EXT_ZBB_TUNE_H
#define HWPROBE_EXT_ZBB_TUNE_H

#include "oeaware/interface.h"

namespace oeaware {
class HwprobeExtZbbTune : public Interface {
public:
    HwprobeExtZbbTune();
    ~HwprobeExtZbbTune() override = default;
    
    Result OpenTopic(const Topic &topic) override;
    void CloseTopic(const Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    void ApplyOptimizations();
    void RevertOptimizations();
    bool CheckHwprobeSupport();
    int64_t GetHwprobeFeature(int64_t key);
    
    // 硬件特性标记
    bool zbbSupported = false;
};
}
#endif // HWPROBE_EXT_ZBB_TUNE_H
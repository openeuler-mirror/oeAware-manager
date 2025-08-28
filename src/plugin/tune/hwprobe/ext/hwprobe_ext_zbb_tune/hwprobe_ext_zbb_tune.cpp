// hwprobe_ext_zbb_tune.cpp
#include "hwprobe_ext_zbb_tune.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <asm/hwprobe.h>
#include <cstring>
#include <fstream>
#include <sstream>

static const char* preloadFile = "/etc/ld.so.preload";
static const char* backupFile  = "/etc/ld.so.preload.bak.hwprobe_ext_zbb_tune";

using namespace oeaware;

HwprobeExtZbbTune::HwprobeExtZbbTune()
{
    name = OE_HWPROBE_EXT_ZBB_TUNE;
    description = "Tuning is carried out using the RISC-V extended instruction set zbb";
    version = "1.0.0";
    period = 1000;
    priority = 2;
    type = oeaware::TUNE;
}

oeaware::Result HwprobeExtZbbTune::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void HwprobeExtZbbTune::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void HwprobeExtZbbTune::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

oeaware::Result HwprobeExtZbbTune::Enable(const std::string &param)
{
    (void)param;
    // 检查硬件支持
    if (!CheckHwprobeSupport())
    {
        return oeaware::Result(FAILED, "The system does not support the hwprobe feature");
    }

    // 获取zbb扩展支持状态
    int64_t ext0 = GetHwprobeFeature(RISCV_HWPROBE_KEY_IMA_EXT_0);
    zbbSupported = ext0 & RISCV_HWPROBE_EXT_ZBB;

    // 采取优化与否
    if (!zbbSupported) {
        return oeaware::Result(FAILED, "zbb extension is not supported");
    }else{
        ApplyOptimizations();
        return oeaware::Result(OK);
    }
}

void HwprobeExtZbbTune::Disable()
{
    // 恢复原始实现
    RevertOptimizations();
}

void HwprobeExtZbbTune::Run()
{
}

void HwprobeExtZbbTune::ApplyOptimizations()
{
    const char* soPath  = "/lib64/oeAware-boost/libzbbtune.so";

    // 检查预编译库是否存在
    std::ifstream soFile(soPath);
    if (!soFile.good()) {
        ERROR(logger, "[HWPROBE_EXT_ZBB] Precompiled library not found: " + std::string(soPath));
        return;
    }

    INFO(logger, "[HWPROBE_EXT_ZBB] Using precompiled Zbb tune library: " + std::string(soPath));

    std::ifstream in(preloadFile);
    std::ofstream bak(backupFile);
    std::ostringstream newContent;
    bool found = false;

    if (in) {
        std::string line;
        while (std::getline(in, line)) {
            if (line == soPath) {
                found = true;
            }
            newContent << line << "\n";
        }
        bak << newContent.str();
        bak.close();
        in.close();
    }

    if (!found) {
        newContent << soPath << "\n";
        std::ofstream out(preloadFile);
        out << newContent.str();
        out.close();
        INFO(logger, "[HWPROBE_EXT_ZBB] Added /etc/oeAware/hwprobe_tune/lib/libzbbtune.so to /etc/ld.so.preload");
    } else {
        INFO(logger, "[HWPROBE_EXT_ZBB] /etc/oeAware/hwprobe_tune/lib/libzbbtune.so already exists in /etc/ld.so.preload");
    }
}

void HwprobeExtZbbTune::RevertOptimizations()
{
    const char* soPath  = "/etc/oeAware/hwprobe_tune/lib/libzbbtune.so";
    std::ifstream in(preloadFile);
    if (!in) {
        INFO(logger, "[HWPROBE_EXT_ZBB] /etc/ld.so.preload not found, nothing to revert");
        return;
    }

    std::ostringstream newContent;
    std::string line;
    bool removed = false;
    while (std::getline(in, line)) {
        if (line == soPath) {
            removed = true;
            continue;
        }
        newContent << line << "\n";
    }
    in.close();

    std::ofstream out(preloadFile);
    out << newContent.str();
    out.close();

    if (removed) {
        INFO(logger, "[HWPROBE_EXT_ZBB] Removed /etc/oeAware/hwprobe_tune/lib/libzbbtune.so from /etc/ld.so.preload");
    } else {
        INFO(logger, "[HWPROBE_EXT_ZBB] /etc/oeAware/hwprobe_tune/lib/libzbbtune.so was not found in /etc/ld.so.preload");
        printf("%s was not found in %s\n", soPath, preloadFile);
    }
}

bool HwprobeExtZbbTune::CheckHwprobeSupport()
{
    std::ifstream file("/usr/include/asm/hwprobe.h");
    return file.good();
}

int64_t HwprobeExtZbbTune::GetHwprobeFeature(int64_t key)
{
    struct riscv_hwprobe pair = {key, 0};
    syscall(SYS_riscv_hwprobe, &pair, 1, 0, NULL, 0);
    return pair.value;
}
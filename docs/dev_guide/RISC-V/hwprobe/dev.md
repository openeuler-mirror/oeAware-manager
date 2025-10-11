# 基于 hwprobe 开发 RISC-V 专有感知与调优实例

## 1. hwprobe 简介

hwprobe 是 RISC-V 架构在 Linux 内核中提供的一套硬件探测接口，允许用户态程序动态查询 CPU 的硬件特性与扩展支持。该接口通过系统调用 `sys_riscv_hwprobe` 实现，使应用程序能够在运行时检测 CPU 功能，从而优化代码路径或验证系统兼容性。更多详细信息可参考https://docs.kernel.org/arch/riscv/hwprobe.html。

openEuler 24.03 (LTS) 作为最新的长期支持版本，搭载 Linux 6.6.0 内核，当前支持以下六个探测键（key）：

- `RISCV_HWPROBE_KEY_MVENDORID`
- `RISCV_HWPROBE_KEY_MARCHID`
- `RISCV_HWPROBE_KEY_MIMPID`
- `RISCV_HWPROBE_KEY_BASE_BEHAVIOR`
- `RISCV_HWPROBE_KEY_IMA_EXT_0`
- `RISCV_HWPROBE_KEY_CPUPERF_0`

## 2. 感知实例

`hwprobe_analysis` 实例与 `hugepage_analysis`、`smc_d_analysis` 等实例类似，在客户端执行 `oeawarectl analysis` 模式时会被订阅。不同之处在于，`hwprobe_analysis` 是专为 RISC-V 架构设计的实例，仅在 RISC-V 平台上生效。

该实例的主要行为包括：

- **检查支持性**：验证系统是否支持 hwprobe 接口，如不支持则使能失败；
- **硬件探测**：通过 `sys_riscv_hwprobe` 系统调用获取 RISC-V 处理器的关键硬件信息；
- **信息转换**：将获取的原始数值转换为可读的字符串描述；
- **结果输出**：通过 Analysis Report 打印探测结果；
- **智能感知**：根据硬件信息推荐可启用的相应调优实例。

### 核心代码解析

#### 硬件探测实现

```cpp
bool HwprobeAnalysis::ProbeHwInfo(std::vector<HwprobeAnalysis::DynamicKey>& keys, 
                                 std::vector<riscv_hwprobe>& pairs) {
    pairs.clear();
    for (const auto& dk : keys) {
        pairs.push_back({dk.key, 0});
    }
    long ret = syscall(SYS_riscv_hwprobe, pairs.data(), pairs.size(), 0, nullptr, 0);
    return ret == 0;
}
```

#### 扩展指令集解析

```cpp
case RISCV_HWPROBE_KEY_IMA_EXT_0: {
    const std::map<uint64_t, std::string> extension_map = {
        {RISCV_HWPROBE_IMA_FD, "F/D (Single/Double Float)"},
        {RISCV_HWPROBE_IMA_C, "C (Compressed)"},
        {RISCV_HWPROBE_IMA_V, "V (Vector)"},
        {RISCV_HWPROBE_EXT_ZBA, "Zba (Address Generation)"},
        {RISCV_HWPROBE_EXT_ZBB, "Zbb (Basic Bit-Manipulation)"},
        {RISCV_HWPROBE_EXT_ZBS, "Zbs (Single-Bit Operations)"}
    };
    std::string desc = "IMA extensions: ";
    for (const auto& [mask, name] : extension_map) {
        if (value & mask) {
            if(mask == RISCV_HWPROBE_EXT_ZBB) {
                ExtSupport.zbb = true; // 设置 zbb 支持标志
            }
            desc += name + " ";
        }
    }
    return desc;
}
```

#### 结果分析与建议生成

```cpp
void HwprobeAnalysis::Analysis(const std::string &topicType) {
    // ... 探测代码 ...
  
    std::string conclusion = "RISC-V hwprobe analysis finished.";
    std::vector<std::string> suggestionItem;
    if(ExtSupport.zbb) {
        conclusion += " Hardware supports Zbb extension. Enabling hwprobe_ext_zbb_tune is recommended.";
        suggestionItem.emplace_back("Zbb extension detected, enable tuning for better performance");
        suggestionItem.emplace_back("oeawarectl -e hwprobe_ext_zbb_tune");
        suggestionItem.emplace_back("This enables optimized utilization of Zbb bit-manipulation instructions.");
    }
    CreateAnalysisResultItem(metrics, conclusion, suggestionItem, type, &analysisResultItem);
}
```

**待办事项（Todo List）**：

- 若未来 openEuler 所采用的内核版本支持更多 hwprobe 键，需同步更新 `ParseHwprobeNote` 函数以兼容新特性。

## 3. 调优实例

目前已针对支持 Zbb 扩展指令集的硬件开发了相应的调优实例 `hwprobe_ext_zbb_tune`。当用户运行 analysis 模式时，若 `hwprobe_analysis` 实例探测到硬件支持 Zbb 扩展指令集，将建议用户启用该调优实例。

### 核心代码解析

#### 硬件支持检查

```cpp
oeaware::Result HwprobeExtZbbTune::Enable(const std::string &param) {
    (void)param;
    // 检查硬件支持
    if (!CheckHwprobeSupport()) {
        return oeaware::Result(FAILED, "The system does not support the hwprobe feature");
    }

    // 获取zbb扩展支持状态
    int64_t ext0 = GetHwprobeFeature(RISCV_HWPROBE_KEY_IMA_EXT_0);
    zbbSupported = ext0 & RISCV_HWPROBE_EXT_ZBB;

    if (!zbbSupported) {
        return oeaware::Result(FAILED, "zbb extension is not supported");
    } else {
        ApplyOptimizations();
        return oeaware::Result(OK);
    }
}
```

#### 动态库拦截实现

```cpp
void HwprobeExtZbbTune::ApplyOptimizations() {
    const char* soPath = "/lib64/oeAware-boost/libzbbtune.so";

    // 检查预编译库是否存在
    std::ifstream soFile(soPath);
    if (!soFile.good()) {
        ERROR(logger, "[HWPROBE_EXT_ZBB] Precompiled library not found: " + std::string(soPath));
        return;
    }

    // 修改 ld.so.preload 文件实现动态库拦截
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
        INFO(logger, "[HWPROBE_EXT_ZBB] Added optimized library to /etc/ld.so.preload");
    }
}
```

启用 `hwprobe_ext_zbb_tune` 实例后，将通过修改 `/etc/ld.so.preload` 文件实现动态库拦截。该机制使得业务程序在运行时自动调用优化后的库函数（利用 Zbb 扩展指令集实现加速），而非系统默认提供的、仅基于基础指令集的版本。

**待办事项（Todo List）**：

- 随着未来 Linux 内核支持更多 hwprobe 探测键，可进一步开发相应的专项调优实例，以覆盖更多硬件特性与优化场景。

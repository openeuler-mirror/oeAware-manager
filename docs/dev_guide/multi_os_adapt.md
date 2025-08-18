# 多版本OS适配
oeAware 当前在低版本OS未发布，或者某些特性依赖特定内核特性(eBPF、SMC...)，低版本难以使用，但仍有较多用户希望在低版本OS（例如4.19内核）下启用部分特性，本文档旨在给出在低版本构建出部分插件的方案。每种OS提供一个单独的章节去描述，难以给出统一的方案。

## 注意事项
1.低版本较多特性未经过充分测试，使用者请自行评估质量。

2. 限于部分OS构建不能在线拉外部代码，所以部分功能需要工程适配，例如将依赖的特定版本包/库/头文件放入某个路径下，指定环境变量构建 oeAware的 rpm.

3.低版本构建流程较多限制，主线难以长期维护。故单独配置 spec
+ 显而易见，这些特殊要求的版本不能在src仓-openEuler库提供spec维护，只能在源码仓提供spec版本。
+ 例如低版本没有libkperf软件包，而 oeAware 较多特性和oeAware强依赖，

## openEuler 2003 SP4
目前eBPF 由于缺少BTF，可以构建出来，无法正常运行。

## kylin V10 SP3
自动化构建路径：`docs\dev_guide\kylinV10SP3`
此路径下脚本，不能直接用于构建环境，构建环境可以参考其进行简单改造。
此脚本运行逻辑：
1. 在openEuler libkperf制品仓下载指定节点版本libkpef构建并安装libkperf，工程团队可以将libkperf引入制品仓。
2. kylin sp3 libbpf 0.8， 配套bpftool 6.8， 下载bpftool，构建并安装至临时路径
3. 环境变量配置bpftool临时路径，打包并构建oeAware。

综上，kylin 可以参照此脚本流程，通过引入libkperf，高版本bpftool，后构建oeAware 的 rpm
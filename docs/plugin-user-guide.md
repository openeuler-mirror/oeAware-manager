
# 简介
本文档主要汇总了 oeAware 插件和实例的信息，并详细介绍了调优实例的使用方式。
其中采集插件和感知插件主要用于采集和整合数据，将数据提供给调优插件，当使用调优插件时，会自动启动调优依赖的采集和感知实例，所以本文档主要侧重介绍调优插件的使用方法。
# 采集插件
## 总览
|采集插件  |实例名 | 功能描述 |2203-LTS-SP4 |2403-LTS-SP1 |
|:---:|:---:|:---:|:---:|:---:|
|libpmu.so| pmu_counting_collector|采集系统PMU性能计数(core)|aarch64|aarch64|
|libpmu.so| pmu_uncore_collector|采集系统PMU性能计数(uncore)|aarch64|aarch64|
|libpmu.so| pmu_sampling_collector|采集系统PMU相关行为记录|aarch64|aarch64|
|libpmu.so| pmu_spe_collector|采集系统SPE记录|aarch64|aarch64|
| libdocker_collector.so |docker_cpu_collector|采集当前环境docker容器的信息|支持|支持|
|libsystem_collector.so|thread_collector|采集当前环境线程信息|支持|支持|
|libsystem_collector.so|kernel_config|采集或配置内核参数|支持|支持|
|libsystem_collector.so|command_collector|使用sysstat 相关采集命令采集系统信息|支持|支持|


# 感知插件
## 总览
|感知插件  |实例名 | 功能描述 |2203-LTS-SP4 |2403-LTS-SP1 |
|:---:|:---:|:---:|:---:|:---:|
| libthread_scenario.so |thread_scenario|感知当前环境关键线程信息|支持|支持|
| libscenario_numa.so（外部插件） |scenario_numa|感知当前环境跨NUMA访存比例|aarch64 | aarch64|
|libanalysis_aware.so |analysis_aware|分析当前环境的业务特征，并给出优化建议|aarch64|aarch64|

# 调优插件

## 总览
|调优插件  |实例名 | 功能描述 |2203-LTS-SP4 |2403-LTS-SP1 |
|:---:|:---:|:---:|:---:|:---:|
|libsystem_tune.so |stealtask_tune|优化CPU调度，减少CPU空转，提供CPU利用率|aarch64|aarch64|
|libsystem_tune.so | smc_tune|基于内核共享内存通信特性，提高网络吞吐，降低时延|不支持|支持|
|libsystem_tune.so | xcall_tune|跳过非关键流程的代码路径，优化 SYSCALL的处理底噪|不支持|aarch64|
|libsystem_tune.so |seep_tune|动态调频，降低整机功耗|aarch64|aarch64|
|libub_tune.so |unixbench_tune|优化UnixBench测试|支持|支持|
|libdocker_tune.so |docker_cpu_burst|在出现突发负载时，CPU Burst可以为容器临时提供额外的CPU资源，缓解CPU限制带来的性能瓶颈，以保障并提升应用（尤其是延迟敏感型应用）的服务质量。|aarch64|aarch64|
|libtune_numa.so (外部插件)| tune_numa_mem_access|周期性迁移线程和内存，减少跨NUMA内存访问|aarch64|aarch64|
| Gazelle| 暂未集成至oeAware中 |高性能用户态协议栈,能够大幅提高应用的网络I/O吞吐能力。专注于数据库网络性能加速|-|-|
| libdfot.so |dfot_tuner_sysboost|动态反馈优化（启动时优化/运行时优化），当前已实现启动时优化功能|aarch64 |不支持|

## 各调优实例使用方式
### tune_numa_mem_access

#### 适用场景
+ 业务访存行为多的场景
+ 业务手动绑核/NUMA有性能提升的场景

#### 前置条件
##### 运行环境
+ aarch64
+ 物理机
+ openEuler kernel (4.19、5.10、6.6)

##### 开启SPE
此插件运行依赖 BIOS 的 SPE ，运行前需要将 SPE 打开。运行`perf list | grep arm_spe`查看是否已经开启SPE，如果开启，则有如下显示
```shell
arm_spe_0//                                      [Kernel PMU event]
```
如果没有开启，则按下述步骤开启。
1. 检查BIOS配置项 MISC Config --> SPE 的状态， 如果状态为 Disable，则需要更改为 Enable。如果找不到这个选项，可能是BIOS版本过低。

2. 进入系统 `vim /etc/grub2-efi.cfg`，定位到内核版本对应的开机启动项，在末尾增加“kpti=off”。例如：
```shell
        linux   /vmlinuz-4.19.90-2003.4.0.0036.oe1.aarch64 root=/dev/mapper/openeuler-root ro rd.lvm.lv=openeuler/root rd.lvm.lv=openeuler/swap video=VGA-1:640x480-32@60me rhgb quiet  smmu.bypassdev=0x1000:0x17 smmu.bypassdev=0x1000:0x15 crashkernel=1024M,high video=efifb:off video=VGA-1:640x480-32@60me kpti=off
```

3. 按“ESC”，输入“:wq”，按“Enter”保存并退出。
4. 执行reboot命令重启服务器。

##### 安装插件
此插件为外部插件，不随 oeAware 一起安装，需要额外安装。
1. 查看此插件是否安装
```shell
oeawarectl -q | grep tune_numa_mem_access
```
2. 如果不存在则按如下步骤安装：
安装方式1:`oeawarectl -i numafast`
安装方式2:在[OEPKGS](https://search.oepkgs.net/zh-CN/list?s=numafast&exactSearch=exact&searchType=default) 选取对应当前系统内核的版本的 numafast 的rpm包，手动进行安装。建议使用最新版本的numafast。
安装后加载此插件 `oeawarectl -l tune_numa_mem_access`

#### 使用方式
1.使能此实例
```shell
oeawarectl -e tune_numa_mem_access
```

2.停止此实例
```shell
oeawarectl -d tune_numa_mem_access
```

### docker_cpu_burst

#### 适用场景
+ 普通容器场景、暂未适配K8S场景
+ 容器内业务负载较高
#### 前置条件
+ 运行环境
    + openEuler kernel (5.10、6.6)
#### 使用方式
1.使能此实例
```shell
oeawarectl -e docker_cpu_burst
```

2.停止此实例
```shell
oeawarectl -d docker_cpu_burst
```
### unixbench_tune
#### 适用场景
UnixBench 测试
#### 使用方式
1.使能此实例
```shell
oeawarectl -e unixbench_tune
```

2.停止此实例
```shell
oeawarectl -d unixbench_tune
```

### stealtask_tune
#### 适用场景
业务负载较高
#### 前置条件
+ 运行环境
    + openEuler kernel (5.10、6.6)
#### 使用方式
1.使能此实例
```shell
oeawarectl -e stealtask_tune
```

2.停止此实例
```shell
oeawarectl -d stealtask_tune
```

### xcall_tune
#### 适用场景
希望降低系统调用开销的场景
#### 前置条件
待补充
#### 使用方式
```shell
oeawarectl -e xcall_tune
```
### seep_tune

#### 适用场景
+ 节能
#### 前置条件
+ aarch64 物理机
+ BIOS 开启 XXX （待补充）
#### 使用方式
1.使能此实例
```shell
oeawarectl -e seep_tune
```

2.停止此实例
```shell
oeawarectl -d seep_tune
```

### smc_tune

#### 适用场景
+ 本地网络通信
#### 前置条件
+ openEuler kernel (6.6)
#### 使用方式
1.使能此实例
```shell
oeawarectl -e smc_tune
```

2.停止此实例
```shell
oeawarectl -d smc_tune
```
### dfot_tuner_sysboost
#### 前置条件
##### 运行环境
+ openEuler kernel (5.10)

##### 安装插件

此插件为外部插件，不随 oeAware 一起安装，需要额外安装。
1. 查看此插件是否安装
```shell
oeawarectl -q | grep dfot_tuner_sysboost
```
2. 如果不存在则按如下步骤安装：
```shell
yum install D-FOT
```

安装后加载此插件 
```shell
oeawarectl -l tune_numa_mem_access
```


#### 使用方式
1.使能此实例
```shell
oeawarectl -e dfot_tuner_sysboost
```

2.停止此实例
```shell
oeawarectl -d dfot_tuner_sysboost
```

### Gazelle

#### 适用场景
+ 业务性能受网络影响
+ 低时延、高吞吐
#### 使用方法
此调优能力暂未集成至oeAware中，请参考 [Gazelle用户指南](https://gitee.com/openeuler/gazelle/blob/master/doc/user-guide.md)


# Overview

This section summarizes the information about the oeAware plugins and instances, and describes how to use the tuning instances.
The collection and sensing plugins are used to collect and integrate data and provide the data for the tuning plugins. When the tuning plugins are used, the collection and awareness instances on which they depend are automatically started. The following mainly describes how to use the tuning plugins.

# Collection Plugins

## Overview

|Collection Plugin |Instance Name| Description|2203-LTS-SP4 |2403-LTS-SP1 |
|:---:|:---:|:---:|:---:|:---:|
|libpmu.so| pmu_counting_collector|Collect the system PMU performance counters (core).|AArch64|AArch64|
|libpmu.so| pmu_uncore_collector|Collect the system PMU performance counters (uncore).|AArch64|AArch64|
|libpmu.so| pmu_sampling_collector|Collect system PMU-related behavior records.|AArch64|AArch64|
|libpmu.so| pmu_spe_collector|Collect system SPE records.|AArch64|AArch64|
| libdocker_collector.so |docker_cpu_collector|Collect Docker container information in the current environment.|Supported|Supported|
|libsystem_collector.so|thread_collector|Collect thread information in the current environment.|Supported|Supported|
|libsystem_collector.so|kernel_config|Collect or configure kernel parameters.|Supported|Supported|
|libsystem_collector.so|command_collector|Use sysstat-related collection commands to collect system information.|Supported|Supported|

# Sensing Plugins

## Overview

|Sensing Plugin |Instance Name| Description|2203-LTS-SP4 |2403-LTS-SP1 |
|:---:|:---:|:---:|:---:|:---:|
| libthread_scenario.so |thread_scenario|Obtain key thread information in the current environment.|Supported|Supported|
| libscenario_numa.so (external plugin)|scenario_numa|Obtain the cross-NUMA memory access ratio in the current environment.|AArch64| AArch64|
|libanalysis_aware.so |analysis_aware|Analyze service characteristics in the current environment and provide tuning suggestions.|AArch64|AArch64|

# Tuning Plugins

## Overview

|Tuning Plugin |Instance Name| Description|2203-LTS-SP4 |2403-LTS-SP1 |
|:---:|:---:|:---:|:---:|:---:|
|libsystem_tune.so |stealtask_tune|Optimize CPU scheduling, reduce CPU idling, and improve CPU utilization.|AArch64|AArch64|
|libsystem_tune.so | smc_tune|Improve the network throughput and reduce the latency based on shared memory communication in the kernel space.|Not supported|Supported|
|libsystem_tune.so | xcall_tune|Bypass non-essential code paths to minimize system call processing overhead.|Not supported|AArch64|
|libsystem_tune.so |seep_tune|Enable dynamic frequency scaling to reduce overall system power consumption.|AArch64|AArch64|
|libub_tune.so |unixbench_tune|Optimize the UnixBench test.|Supported|Supported|
|libdocker_tune.so |docker_cpu_burst|CPUBurst can temporarily provide additional CPU resources for containers to alleviate performance bottlenecks caused by CPU limits when burst loads occur. This ensures and improves the service quality of applications (especially latency-sensitive applications).|AArch64|AArch64|
|libtune_numa.so (external plugin)| tune_numa_mem_access|Periodically migrate threads and memory to reduce cross-NUMA memory access.|AArch64|AArch64|
| Gazelle| Not integrated into oeAware|The high-performance user-mode protocol stack greatly improves the network I/O throughput of applications and focuses on database network performance acceleration.|-|-|
| libdfot.so |dfot_tuner_sysboost|Dynamic feedback optimization (optimization at startup and runtime). Currently, the optimization at startup is implemented.|AArch64|Not supported|

## Tuning Instance Usage

### tune_numa_mem_access

#### Application Scenarios

+ Frequent memory access
+ Performance gains from manual core or NUMA affinity setting

#### Prerequisites

##### Operating Environment

+ AArch64
+ Physical machine
+ openEuler kernel (4.19, 5.10, 6.6)

##### Enabling SPE

This plugin relies on the BIOS SPE feature. Before running the plugin, you need to enable the SPE. Run `perf list | grep arm_spe` to check whether the SPE is enabled. If it is enabled, the following information is displayed:

```shell
arm_spe_0//                                      [Kernel PMU event]
```

If not, perform the following steps to enable it:

1. Go to MISC Config --> SPE in the BIOS. If the SPE is set to `Disable`, switch it to `Enable`. If you cannot find this option, the BIOS version may be outdated.

2. Access `vim /etc/grub2-efi.cfg` of the system, locate the startup item corresponding to the kernel version, and add `kpti=off` to the end of the startup item. Example:

    ```shell
            linux   /vmlinuz-4.19.90-2003.4.0.0036.oe1.aarch64 root=/dev/mapper/openeuler-root ro rd.lvm.lv=openeuler/root rd.lvm.lv=openeuler/swap video=VGA-1:640x480-32@60me rhgb quiet  smmu.bypassdev=0x1000:0x17 smmu.bypassdev=0x1000:0x15 crashkernel=1024M,high video=efifb:off video=VGA-1:640x480-32@60me kpti=off
    ```

3. Press `Esc`, enter `:wq`, and press `Enter` to save the change and exit.
4. Run the `reboot` command to restart the server.

##### Installing the Plugin

This plugin is an external plugin and is not installed together with oeAware. You need to install it separately.

1. Check whether the plugin is installed.

    ```shell
    oeawarectl -q | grep tune_numa_mem_access
    ```

2. If the plugin does not exist, install it as follows:

    Installation method 1: `oeawarectl -i numafast`

    Installation method 2: Select the RPM package of numafast corresponding to the current system kernel version from [OEPKGS](https://search.oepkgs.net/en-US/list?s=numafast&exactSearch=exact&searchType=default) and manually install it. You are advised to use the latest version of numafast.

    After installation, load the plugin by running `oeawarectl -l tune_numa_mem_access`.

#### How to Use

1. Enable this instance.

    ```shell
    oeawarectl -e tune_numa_mem_access
    ```

2. Stop this instance.

    ```shell
    oeawarectl -d tune_numa_mem_access
    ```

### docker_cpu_burst

#### Application Scenarios

+ Common containers, not applicable to K8s
+ High service load in the container

#### Prerequisites

+ Operating Environment
    + openEuler kernel (5.10, 6.6)

#### How to Use

1. Enable this instance.

    ```shell
    oeawarectl -e docker_cpu_burst
    ```

2. Stop this instance.

    ```shell
    oeawarectl -d docker_cpu_burst
    ```

### unixbench_tune

#### Application Scenarios

UnixBench test

#### How to Use

1. Enable this instance.

    ```shell
    oeawarectl -e unixbench_tune
    ```

2. Stop this instance.

    ```shell
    oeawarectl -d unixbench_tune
    ```

### stealtask_tune

#### Application Scenarios

High service load

#### Prerequisites

+ Operating Environment
    + openEuler kernel (5.10, 6.6)

#### How to Use

1. Enable this instance.

    ```shell
    oeawarectl -e stealtask_tune
    ```

2. Stop this instance.

    ```shell
    oeawarectl -d stealtask_tune
    ```

### xcall_tune

#### Application Scenarios

System call overhead reduction

#### Prerequisites

To be added.

#### How to Use

```shell
oeawarectl -e xcall_tune
```

### seep_tune

#### Application Scenarios

+ Energy saving

#### Prerequisites

+ AArch64 physical machine
+ XXX enabled on the BIOS (to be added)

#### How to Use

1. Enable this instance.

    ```shell
    oeawarectl -e seep_tune
    ```

2. Stop this instance.

    ```shell
    oeawarectl -d seep_tune
    ```

### smc_tune

#### Application Scenarios

+ Local network communication

#### Prerequisites

+ openEuler kernel (6.6)

#### How to Use

1. Enable this instance.

    ```shell
    oeawarectl -e smc_tune
    ```

2. Stop this instance.

    ```shell
    oeawarectl -d smc_tune
    ```

### dfot_tuner_sysboost

#### Prerequisites

##### Operating Environment

+ openEuler kernel (5.10)

##### Installing the Plugin

This plugin is an external plugin and is not installed together with oeAware. You need to install it separately.

1. Check whether the plugin is installed.

    ```shell
    oeawarectl -q | grep dfot_tuner_sysboost
    ```

2. If the plugin does not exist, install it.

    ```shell
    yum install D-FOT
    ```

    Load the plugin after installation.

    ```shell
    oeawarectl -l tune_numa_mem_access
    ```

#### How to Use

1. Enable this instance.

    ```shell
    oeawarectl -e dfot_tuner_sysboost
    ```

2. Stop this instance.

    ```shell
    oeawarectl -d dfot_tuner_sysboost
    ```

### Gazelle

#### Application Scenarios

+ Service performance affected by the network
+ Low latency and high throughput

#### How to Use

This tuning capability has not been integrated into oeAware. For details, see [Gazelle User Guide](https://gitcode.com/openeuler/gazelle/blob/master/doc/en/user-guide_en.md).

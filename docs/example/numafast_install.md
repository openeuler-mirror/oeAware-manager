# numafast 调优工具介绍
## 介绍
numafast 是一个用于鲲鹏芯片的用户态调优工具，旨在减少系统的跨NUMA内存访问，提高系统性能。

## 使用限制

+ 仅鲲鹏可用，目前代码闭源、软件二进制开源。
+ 仅可在物理机运行，无法在虚拟机中运行。
+ 运行依赖 SPE ，SPE 是一种硬件辅助CPU分析机制， 运行前需要将SPE打开。
+ root 下运行

## 使能方法

### 开启SPE

运行 perf list | grep arm_spe 查看当前是否已经开启SPE，如果开启，则有如下显示：
```shell
[root@master ~]# perf list | grep arm_spe
  arm_spe_0//                                        [Kernel PMU event]
```
如果没有上述内容，表示SPE未打开，可按以下步骤开启SPE（kunpeng 920 高性能不需要手动配置，默认支持）。

检查BIOS配置项 MISC Config--> SPE的状态，如果状态为Disabled需要更改为Enabled。如果找不到这个选项，可能是BIOS版本过低。

### 安装
numafast目前在OEPKGS网站上发布：
https://search.oepkgs.net/zh-CN/list?s=numafast&exactSearch=exact&searchType=default


可选取最高版本（第二列）运行，每个版本号目前对应这三个系统版本，分别对应4.19、5.10、6.6内核。系统版本号和当前OS版本无需完全匹配，只要内核匹配即可，例如，22.03-LTS-SP4 系统版本的软件包可以运行在5.10内核的各系统版本上。

下载后得到一个rpm包，在环境上安装 `rpm -ivh numafast-xxx.rpm`

rpm 安装后会在  `/usr/bin/` 下添加 `numafast` 二进制命令。
卸载命令：`rpm -e numafast`

### 运行
可选择以下任意一种方式运行。

#### 方式1：采用二进制方式运行
Numafast是一个动态调优的程序，会持续调优，所以需要在业务运行期间一直运行。

终端执行  numafast , 此时会一直运行，CTRL+C 可退出运行。

如果想后台执行 nohup numafast  &, 退出的时候请不要 kill -9 pid , 请用 kill -2 , 因为nuamfast要做一些复位操作。Kill -9 不会执行复位流程（会导致绑核未恢复解绑）。

上述方式建议临时调试使用，在v241-1版本之后，numafast支持systemd方式启动
```shell
systemctl start numafast
```

#### 方式2：作为 oeAware 插件启动
nuamfast既可以独立二进制运行，又可以作为oeAware的插件使用，oeAware是openEuler 开源调优框架，在安装numafast后，安装oeAware
```shell
yum install oeAware-manager
```
启动：
```shell
systemctl start oeaware
```
使能numa调优：
```shell
oeaware -e tune_numa_mem_access
```
去使能numa调优
```shell
oeawarectl -d tune_numa_mem_access
```
oeaware 使能方式和numafast 二进制使能方式对调优而言无本质差别，但后续会oeaware会逐渐推广。
oeAware 相关使用方法请参考oeAware仓库文档：https://gitee.com/openeuler/oeAware-manager

## 注意事项
###  Numa balancing
Numa balancing 是 linux 的一个自带的numa调优功能，通常开箱是默认开启的，numafast开启后会自动将此功能关闭，退出时会恢复回启动前的状态。

###  业务绑核
程序目前会强制迁移所有程序(或只迁移白名单的程序、不迁移黑名单的程序)，如果预先对业务做了绑核等操作，numafast仍会迁移业务。

###  部分环境不适配
Numafast开发时是openeuler 环境下验证，其他OS可能无法正常运行。

### 维护方法
如果遇到此软件的相关问题，可以在社区提issue解决：

https://gitee.com/src-oepkgs/numafast/issues
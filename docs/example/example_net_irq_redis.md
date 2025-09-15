## 环境准备

|依赖| 版本|
|--|--|
|服务器|2台 kunpeng 920|
|OS|openEuler 2403 LTS SP1|
|网卡|hinic|
|oeAware-manager|>=v2.1.0-1|
|numafast|>=v2.4.1-1|
|redis|6.2.7|

安装numactl 和关闭防火墙、selinux
```shell
yum install numactl
systemctl stop firewalld.service
setenforce 0
```

两台服务器网卡直连，保证网络稳定。

## 准备 redis

```shell
wget https://download.redis.io/releases/redis-6.2.7.tar.gz --no-check-certificate
tar -zxvf redis-6.2.7.tar.gz
cd redis-6.2.7
make -j 32
```
修改redis配置文件
```shell
protected-mode no # yes 改为 no
# bind 127.0.0.1 -::1 # 注释掉
```
## 安装 oeAware
```shell
[root@master ~]# yum install oeAware-manager
```
本台环境的网卡队列在`/proc/interrupts`中名字为 `enp25s0_qpxxx`,
插件配置文件中新增一行 `vim /etc/oeAware/plugin/hardirq_tune.conf`
```shell
REGEXP:VAR_ETH_qpVAR_QUEUE
```
表示该环境中断号和队列号的映射关系
## 测试场景
测试 Redis 在高并发（32 个连接）下处理 500,000 次 set 和 get 命令的性能。
通过 -r 500000 避免缓存命中，更贴近真实场景。使用 32 个连接和 6 个线程模拟多线程并发请求。-l 保持测试持续进行，直到手动停止。

## 基线测试
停止所有调优
```shell
systemctl stop oeaware
```
启动redis服务端和客户端测试基线数据，注意，此优化项非NUMA调优，为了减少线程在NUMA间调度对性能的影响，测试时将服务端和客户端绑定在各自机器的某个NUMA上。客户端的`-h`参数请设置为服务端的ip。
```shell
[root@master redis-6.2.7]# numactl -N 0 ./src/redis-server ./redis.conf
[root@master redis-6.2.7]# numactl -N 0 ./src/redis-benchmark -t set,get -c 32 -n 500000 -r 500000 --threads 6 -l -q --csv -h 192.168.100.2
"test","rps","avg_latency_ms","min_latency_ms","p50_latency_ms","p95_latency_ms","p99_latency_ms","max_latency_ms"

```
部分结果如下，
```shell
[root@master redis-6.2.7]# numactl -N 0 ./src/redis-benchmark -t set,get -c 32 -n 500000 -r 500000 --threads 6 -l -q --csv -h 192.168.100.2
"test","rps","avg_latency_ms","min_latency_ms","p50_latency_ms","p95_latency_ms","p99_latency_ms","max_latency_ms"
"SET","86926.29","0.354","0.080","0.335","0.463","0.495","1.927"
"GET","105218.86","0.298","0.072","0.279","0.407","0.487","4.071"
"SET","111086.43","0.275","0.056","0.255","0.367","0.463","8.239"
"GET","99960.02","0.303","0.056","0.279","0.407","0.503","1.191"
"SET","105218.86","0.297","0.064","0.271","0.399","0.495","1.175"
"GET","111061.76","0.277","0.056","0.255","0.367","0.463","8.247"
"SET","111061.76","0.282","0.136","0.263","0.375","0.471","0.951"
"GET","105218.86","0.288","0.048","0.263","0.383","0.487","1.079"
"SET","105241.00","0.289","0.064","0.263","0.383","0.479","1.039"
"GET","105218.86","0.286","0.104","0.263","0.383","0.479","6.055"


```
## 调优测试
启动 oeaware 的 net_hard_irq_tune 调优插件进行中断调优。

```shell
[root@master ~]# systemctl start oeaware
[root@master ~]# oeawarectl -e net_hard_irq_tune
Instance enabled successfully.
```
启动redis服务端和客户端,进行性能测试。
```shell
[root@master redis-6.2.7]# numactl -N 0 ./src/redis-server ./redis.conf
...
[root@master redis-6.2.7]# numactl -N 0 ./src/redis-benchmark -t set,get -c 32 -n 500000 -r 500000 --threads 6 -l -q --csv
"test","rps","avg_latency_ms","min_latency_ms","p50_latency_ms","p95_latency_ms","p99_latency_ms","max_latency_ms"
"GET","222123.52","0.132","0.040","0.135","0.175","0.191","0.727"
"SET","249875.08","0.118","0.040","0.119","0.151","0.159","0.855"
"GET","249875.08","0.112","0.032","0.111","0.143","0.151","0.383"
"SET","249875.08","0.118","0.032","0.119","0.151","0.159","0.663"
"GET","249875.08","0.114","0.032","0.119","0.151","0.159","0.383"
"SET","249875.08","0.118","0.032","0.119","0.151","0.159","0.839"
"GET","249875.08","0.113","0.032","0.119","0.151","0.159","0.327"
"SET","249875.08","0.116","0.032","0.119","0.151","0.159","0.815"
"GET","249875.08","0.113","0.032","0.119","0.151","0.159","0.383"
"SET","249875.08","0.118","0.032","0.119","0.151","0.159","0.719"
"GET","249875.08","0.112","0.040","0.111","0.143","0.151","0.431"
"SET","250000.00","0.117","0.040","0.119","0.151","0.159","0.823"
```
总结：redis 本地测试，启动oeaware的net_hard_irq_tune进行本地网络调优后，get 性能提升（105335.672->245249.82，57%）, set 性能提升 (103906.868->249895.9,58%)

注意：性能提升和业务的初始状态有有关，例如如果业务初始状态下和网卡中断以及网卡在相同的NUMA，那么性能提升的空间就没有多少。
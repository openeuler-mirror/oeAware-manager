## 环境准备

|依赖| 版本|
|--|--|
|服务器|kunpeng 920|
|OS|openEuler 2403 LTS SP1|
|oeAware-manager|>=v2.1.0-1|
|numafast|>=v2.4.1-1|
|redis|6.2.7|

安装numactl 和关闭防火墙、selinux
```shell
yum install numactl
systemctl stop firewalld.service
setenforce 0
```

## 准备 redis

```shell
wget https://download.redis.io/releases/redis-6.2.7.tar.gz --no-check-certificate
tar -zxvf redis-6.2.7.tar.gz
cd redis-6.2.7
make -j 32
```
## 安装 oeAware
```shell
[root@master ~]# yum install oeAware-manager
```

## 测试场景
测试 Redis 在高并发（32 个连接）下处理 500,000 次 set 和 get 命令的性能。
通过 -r 500000 避免缓存命中，更贴近真实场景。使用 32 个连接和 6 个线程模拟多线程并发请求。-l 保持测试持续进行，直到手动停止。

##


## 基线测试
停止所有调优
```shell
systemctl stop oeaware
```
启动redis服务端和客户端测试基线数据，注意，此优化项非NUMA调优，为了减少线程在NUMA间调度对性能的影响，测试时将服务端和客户端绑定在相同NUMA上。
```shell
[root@master redis-6.2.7]# numactl -N 0 ./src/redis-server ./redis.conf
[root@master redis-6.2.7]# numactl -N 0 ./src/redis-benchmark -t set,get -c 32 -n 500000 -r 500000 --threads 6 -l -q --csv
```
部分结果如下，
```shell
[root@master redis-6.2.7]# numactl -N 0 ./src/redis-benchmark -t set,get -c 32 -n 500000 -r 500000 --threads 6 -l -q --csv
"test","rps","avg_latency_ms","min_latency_ms","p50_latency_ms","p95_latency_ms","p99_latency_ms","max_latency_ms"
"SET","117591.73","0.259","0.064","0.255","0.327","0.503","1.967"
"SET","117619.38","0.260","0.032","0.255","0.335","0.503","2.127"
"SET","117619.38","0.259","0.040","0.255","0.335","0.503","2.471"
"SET","117619.38","0.260","0.048","0.255","0.335","0.503","2.023"
"SET","117619.38","0.260","0.040","0.255","0.335","0.503","1.255"
"SET","117619.38","0.258","0.040","0.255","0.327","0.503","2.055"

```
## 调优测试
启动 oeaware 的 smc_tune 调优插件进行numa调优

```shell
[root@master ~]# systemctl start oeaware
[root@master ~]# oeawarectl -e smc_tune
Instance enabled successfully.
```
启动性能测试，注意需要重启redis服务端
```shell
[root@master redis-6.2.7]# numactl -N 0 ./src/redis-server ./redis.conf
...
[root@master redis-6.2.7]# numactl -N 0 ./src/redis-benchmark -t set,get -c 32 -n 500000 -r 500000 --threads 6 -l -q --csv
"test","rps","avg_latency_ms","min_latency_ms","p50_latency_ms","p95_latency_ms","p99_latency_ms","max_latency_ms"
"SET","181752.09","0.168","0.016","0.159","0.247","0.319","1.815"
"GET","181752.09","0.165","0.024","0.159","0.263","0.319","1.735"
"SET","166666.67","0.174","0.016","0.167","0.255","0.327","1.895"
"GET","181818.19","0.165","0.032","0.159","0.263","0.319","0.615"
"SET","166611.14","0.171","0.024","0.159","0.255","0.327","1.807"
"GET","181818.19","0.163","0.024","0.159","0.247","0.319","0.543"
"SET","166611.14","0.172","0.016","0.167","0.263","0.327","1.911"
```
总结：redis 本地测试，启动oeaware的smc_tune进行本地网络调优后，get 性能提升（120069.1733->181796.1567,	33.95%）, set 性能提升 (117619.38	171676.6333->31.48%)

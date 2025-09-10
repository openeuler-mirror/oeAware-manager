## 环境准备

|依赖| 版本|
|--|--|
|服务器|kunpeng 920|
|OS|openEuler 2203 LTS SP4|
|oeAware-manager|>=v2.1.0-1|
|numafast|>=v2.4.1-1|
|redis|6.2.7|

## 安装 redis

```shell
wget https://download.redis.io/releases/redis-6.2.7.tar.gz --no-check-certificate
tar -zxvf redis-6.2.7.tar.gz
cd redis-6.2.7
make -j 32
```

## 安装 numafast
参考 [numafast 调优工具介绍 ](./numafast_install.md)

## 安装 oeAware
```shell
[root@master ~]# yum install oeAware-manager
```

## 测试场景
测试 Redis 在高并发（32 个连接）下处理 500,000 次 set 和 get 命令的性能。
通过 -r 500000 避免缓存命中，更贴近真实场景。使用 32 个连接和 6 个线程模拟多线程并发请求。-l 保持测试持续进行，直到手动停止。
## 基线测试
停止所有调优
```shell
systemctl stop oeaware
```
启动redis服务端和客户端测试基线数据
```shell
./src/redis-server ./redis.conf
./src/redis-benchmark -t set -c 32 -n 500000 -r 500000 --threads 6 -l -q --csv
```
部分结果如下，
```shell
[root@master redis-6.2.7]# ./src/redis-benchmark -t set,get -c 32 -n 500000 -r 500000 --threads 6 -l -q --csv
"test","rps","avg_latency_ms","min_latency_ms","p50_latency_ms","p95_latency_ms","p99_latency_ms","max_latency_ms"
"GET","79974.41","0.382","0.024","0.375","0.471","0.735","2.375"
"SET","83305.57","0.374","0.032","0.367","0.479","0.735","2.071"
"GET","86926.29","0.351","0.032","0.343","0.439","0.671","1.247"
"SET","83305.57","0.374","0.032","0.359","0.471","0.711","2.167"
"GET","83305.57","0.366","0.032","0.359","0.455","0.703","2.775"
"SET","86941.41","0.356","0.032","0.351","0.447","0.671","1.159"
"GET","99960.02","0.305","0.032","0.287","0.383","0.559","2.063"
"SET","83319.45","0.363","0.032","0.351","0.463","0.695","1.991"
"GET","99980.01","0.304","0.024","0.287","0.383","0.551","2.111"
"SET","95219.96","0.324","0.032","0.295","0.495","0.599","2.591"
```
## 调优测试
启动 oeaware 的 tune_numa_mem_access 调优插件进行numa调优

```shell
[root@master ~]# systemctl start oeaware
[root@master ~]# oeawarectl -e tune_numa_mem_access
Instance enabled successfully.
```
启动性能测试
```shell
[root@master redis-6.2.7]# ./src/redis-benchmark -t set,get -c 32 -n 500000 -r 500000 --threads 6 -l -q --csv
"test","rps","avg_latency_ms","min_latency_ms","p50_latency_ms","p95_latency_ms","p99_latency_ms","max_latency_ms"
"GET","105241.00","0.287","0.032","0.287","0.359","0.391","0.775"
"SET","105241.00","0.298","0.032","0.295","0.383","0.415","0.791"
"GET","105218.86","0.285","0.032","0.279","0.359","0.391","0.719"
"SET","105241.00","0.297","0.032","0.295","0.383","0.423","0.791"
"GET","105241.00","0.296","0.032","0.287","0.359","0.479","0.919"
"SET","105218.86","0.297","0.032","0.295","0.383","0.415","0.767"
"GET","105241.00","0.287","0.032","0.287","0.359","0.391","0.799"
"SET","105241.00","0.297","0.032","0.295","0.383","0.415","4.159"
```
总结：redis 本地测试，启动oeaware的tune_numa_mem_access进行NUMA调优后，get 性能提升（90029.26->105235.465, 14.4%）, set 性能提升 (86418.392->105235.465, 21.8%)

备注：基线测试时，因为redis服务端和客户端可能在不同NUMA节点，也可能在相同NUMA节点，通常基线性能不稳定，所以NUMA调优的收益也不是稳定的。
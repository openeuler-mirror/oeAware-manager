## 环境准备

|依赖| 版本|
|--|--|
|服务器|kunpeng 920|
|OS|openEuler 2403 LTS SP1|
|oeAware-manager|>=v2.1.0-1|
|numafast|>=v2.4.1-1|


## 安装nginx
```shell
[root@master ~]# yum install nginx wrk
[root@master ~]# nginx -v
nginx version: nginx/1.24.0
```

## 安装 numafast
参考 [numafast 调优工具介绍 ](./numafast_install.md)

## 安装 oeAware
```shell
[root@master ~]# yum install oeAware-manager
```

## 测试用例
通过 100 个并发连接和 20 个线程，模拟真实场景下的高并发请求，测试服务器的吞吐量（Requests Per Second, RPS）和资源占用情况。
## 基线测试
停止所有调优
```shell
systemctl stop oeaware
```
启动服务端
```shell
[root@master ~]#  /usr/sbin/nginx -c /etc/nginx/nginx.conf
```
启动压测端
```shell
[root@master ~]# wrk -c 100 -d 10s -t 20 --latency --timeout 5s -H "Connection: keep-alive" http://127.0.0.1:80/index.html
Running 10s test @ http://127.0.0.1:80/index.html
  20 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   790.22us  118.46us   7.26ms   85.10%
    Req/Sec     6.34k   413.16     8.18k    77.13%
  Latency Distribution
     50%  785.00us
     75%  824.00us
     90%    0.88ms
     99%    1.23ms
  1275015 requests in 10.10s, 4.45GB read
Requests/sec: 126244.32
Transfer/sec:    451.36MB
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
[root@master ~]# wrk -c 100 -d 10s -t 20 --latency --timeout 5s -H "Connection: keep-alive" http://127.0.0.1:80/index.html
Running 10s test @ http://127.0.0.1:80/index.html
  20 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   707.54us  285.86us  21.44ms   97.21%
    Req/Sec     7.13k   360.65     9.79k    88.11%
  Latency Distribution
     50%  699.00us
     75%  723.00us
     90%  768.00us
     99%    0.94ms
  1433183 requests in 10.10s, 5.00GB read
Requests/sec: 141907.32
Transfer/sec:    507.36MB
```
总结：nginx 本地测试，启动oeaware的tune_numa_mem_access 进行NUMA调优后，requests性能提升（1275015->1433183,11%）

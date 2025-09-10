

## 环境准备

|依赖| 版本|
|--|--|
|服务器|kunpeng 920|
|OS|openEuler 2403 LTS SP1|
|oeAware-manager|>=v2.1.0-1|

关闭防火墙、selinux
```shell
systemctl stop firewalld.service
setenforce 0
```

## 安装nginx
```shell
[root@master ~]# yum install nginx wrk
[root@master ~]# nginx -v
nginx version: nginx/1.24.0
```

修改配置文件`/etc/nginx/nginx.conf`为4个进程
```shell
worker_processes 4;
```

## 安装 oeAware
```shell
[root@master ~]# yum install oeAware-manager
```

## 测试用例
通过 100 个并发连接和 20 个线程，模拟真实场景下的高并发请求，测试服务器的吞吐量（Requests Per Second, RPS）和资源占用情况。


## 基线数据测试
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

## 本地网络调优测试
开启smc_tune优化
```shell
[root@master ~]# systemctl start oeaware
[root@master ~]# oeawarectl -e smc_tune
```
注意重启nginx服务端后启动压测
```shell
[root@master ~]# /usr/sbin/nginx -c /etc/nginx/nginx.conf
[root@master ~]# wrk -c 100 -d 10s -t 20 --latency --timeout 5s -H "Connection: keep-alive" http://127.0.0.1:80/index.html
Running 10s test @ http://127.0.0.1:80/index.html
  20 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   680.58us   79.91us   4.27ms   82.61%
    Req/Sec     7.36k   426.21    11.03k    85.19%
  Latency Distribution
     50%  681.00us
     75%  709.00us
     90%  752.00us
     99%    0.92ms
  1477713 requests in 10.10s, 5.16GB read
Requests/sec: 146316.43
Transfer/sec:    523.13MB
```

总结：nginx 本地测试，启动oeaware的smc_tune进行本地网络调优后，requests性能提升（1275015->1477713,13.7%）
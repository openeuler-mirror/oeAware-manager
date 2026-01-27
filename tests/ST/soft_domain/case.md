## 测试脚本说明

### gen_docker.sh
 创建docker 并在创建结束后，周期打印docker的soft_domain参数，可以在使能优化前后观察参数变化。
docker启动后会执行load.cpp 程序，会启动一个单进程多线程负载业务，可以观察业务的负载是否亲和cluster。
```shell
sh gen_docker.sh 4 8 12 test
```
load 程序会带有docker名字参数，方便观测

### del_docker.sh 
 批量删除docker
```shell
sh del_docker.sh test # 删除docker名字带有 test的docker
```
### show_docker.sh
周期显示环境docker的soft_domain 相关参数。
```shell
sh show_docker.sh
```

### 其他辅助脚本

```shell
ps -T -p 1234 -o pid,tid,psr,comm # 观察选核
```


## 测试用例

1、docker基础场景测试：96 核环境，生成配额为4的 12 个docker，平均绑在各个numa， 先启动docker，后使能soft_domain，测试配额4，使能后再反复使能和去使能。
```shell
sh gen_docker.sh 4 8 12 test
sh show_docker.sh
oeawarectl -e soft_domain_tune
oeawarectl -d soft_domain_tune
oeawarectl -e soft_domain_tune
oeawarectl -d soft_domain_tune
sh del_docker.sh test
```
配置文件
```yaml
 - type: "docker"
   whitelist: ["test*", "redis*"]
   cpu_num: "8"
```


2、docker 基础场景测试: 96 核环境，生成配额为4的 12 个docker，平均绑在各个numa， 先使能soft_domain，后启动docker, 测试配额4 。

```shell 
oeawarectl -e soft_domain_tune
sh gen_docker.sh 4 8 12 test
oeawarectl -d soft_domain_tune
oeawarectl -e soft_domain_tune
oeawarectl -d soft_domain_tune
sh del_docker.sh test
```
配置文件
```yaml
 - type: "docker"
   whitelist: ["test*", "redis*"]
   cpu_num: "8"
```


3、docker 基础场景测试：96 核环境，生成配额为4的 12 个docker，平均绑在各个numa， 使能 soft_domain， 然后删除2各docker，再创建两个docker

```shell 
sh gen_docker.sh 4 8 12 test
oeawarectl -e soft_domain_tune
docker rm -f id1
docker rm -f id2
sh gen_docker.sh 4 8 2 test
sh del_docker.sh test
```

配置文件
```yaml
 - type: "docker"
   whitelist: ["test*", "redis*"]
   cpu_num: "8"
```

4、docker 基础场景测试：96 核环境，生成配额为4的 12 个docker，不绑定numa， 使能 soft_domain

```shell
sh gen_docker.sh 4 0 12 test
oeawarectl -e soft_domain_tune
oeawarectl -d soft_domain_tune
sh del_docker.sh test
```

配置文件

```yaml
 - type: "docker"
   whitelist: ["test*", "redis*"]
   cpu_num: "8"
```


5、docker 基础场景测试：96 核环境，生成配额为4的 12 个docker，不绑定numa， 使能 soft_domain，删除全部docker，再创建

```shell
sh gen_docker.sh 4 0 12 test
oeawarectl -e soft_domain_tune
sh del_docker.sh test
sh gen_docker.sh 4 0 12 test
oeawarectl -d soft_domain_tune
sh del_docker.sh test
```

配置文件

```yaml
 - type: "docker"
   whitelist: ["test*", "redis*"]
   cpu_num: "8"
```

6、生成一部分绑定numa的docker，再申请一部分不绑定numa的docker，然后再删除绑定numa的docker，再创建绑定numa的docker

```shell
sh gen_docker.sh 4 8 12 bind
sh gen_docker.sh 4 0 12 unb
oeawarectl -e soft_domain_tune
sh del_docker.sh bind
sh gen_docker.sh 4 8 12 bind
oeawarectl -d soft_domain_tune
sh del_docker.sh bind
sh del_docker.sh unb

```

配置文件

```yaml
 - type: "docker"
   whitelist: ["bind*", "unbind*"]
   cpu_num: "8"
```


7、生成一部分绑定numa的docker，再申请一部分不绑定numa的docker，然后再删除绑定numa的docker，再创建绑定numa的docker

```shell
sh gen_docker.sh 4 8 12 bind
sh gen_docker.sh 4 0 12 unb
oeawarectl -e soft_domain_tune
sh del_docker.sh bind
sh gen_docker.sh 4 8 12 bind
oeawarectl -d soft_domain_tune
sh del_docker.sh bind
sh del_docker.sh unb

```

配置文件空

8 进程测试，生成16个 load 进程，然后再删除8个，再创建8个

```shell
g++ load.cpp -o loadn
g++ load.cpp -o loadm
for i in {1..16}; do { ./loadn 3 $i & }; done
for i in {1..16}; do { ./loadm 3 $i & }; done
oeawarectl -e soft_domain_tune
pkill loadm
for i in {1...16}; do load m; done
oeawarectl -d soft_domain_tune
```
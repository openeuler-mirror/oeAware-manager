# oeAware-manager

#### 介绍
oeAware是在openEuler上实现低负载采集、感知、调优的框架，目标是动态感知系统行为后智能使能系统的调优特性。

#### 软件架构
软件架构说明

支持arm和X86
#### 安装教程
##### yum安装
```sh
yum install oeAware-manager
```
安装完成后，通过以下命令查看是否安装成功。
```sh
systemctl status oeaware
```
##### 源码编译运行
依赖安装
```sh
yum-builddep oeAware.spec
```
编译
```sh
mkdir build
cd build
cmake ..
make 
```
运行
```sh
chmod 640 config.yaml
./build/src/plugin_mgr/oeaware config.yaml
```
#### 文档

+ [oeAware使用指南](docs/zh/master/oeaware_user_guide.md)
+ [oeAware-设计文档](docs/design/oeaware-设计文档.md)
+ [oeAware 使用案例](docs/example/)

# oeAware-manager

#### Introduction
oeAware is a framework for low load acquisition, sensing, and tuning on openEuler, with the goal of dynamically sensing the behavior of the system and then intelligently enabling the tuning characteristics of the system.

#### Software Architecture
Software Architecture

Support arm and X86
#### Installation Tutorial
##### yum install
```sh
yum install oeAware-manager
```
After the installation is complete, check if the installation was successful by using the following command.
```sh
systemctl status oeaware
```
##### source compile and run
Dependent installation
```sh
yum-builddep oeAware.spec
```
Compile
```sh
mkdir build
cd build
cmake ...
make 
```
Run
```sh
chmod 640 config.yaml
. /build/src/plugin_mgr/oeaware config.yaml
```
#### Usage Notes

[oeAware User's Guide](docs/oeAware用户指南.md)


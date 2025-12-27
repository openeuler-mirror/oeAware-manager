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
bash build.sh
```
Run
```sh
./build/output/bin/oeaware etc/config.yaml
```
#### Usage Notes

[oeAware User's Guide](docs/en/master/oeaware_user_guide.md)


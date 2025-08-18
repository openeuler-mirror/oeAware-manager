#!/bin/bash
set -x
yum install -y git rsync
yum install -y rpmdevtools

spec_file="oeAware-manager.spec"
script_dir=$(cd $(dirname $0);pwd)
cd $script_dir/
name="oeAware-manager"
version=$(awk '/Version:/{print $2}' "$spec_file")

yum-builddep $spec_file
rpmdev-setuptree

# 安装libkperf
cd $script_dir/
rm -rf libkperf
git clone https://gitee.com/src-openeuler/libkperf.git
cd libkperf/
git reset --hard 75a943cc939971fedc9576f35d81c845213ab483
cp *.patch /root/rpmbuild/SOURCES/
cp libkperf-v1.2.tar.gz /root/rpmbuild/SOURCES/
yum-builddep libkperf.spec
rpmbuild -ba libkperf.spec
rpm -ivh /root/rpmbuild/RPMS/aarch64/libkperf-v1.2-2.*.rpm
rpm -ivh /root/rpmbuild/RPMS/aarch64/libkperf-devel-v1.2-2.*.rpm


# 构建特定版本的bpftool, libbpf 0.8 <-> bpftool 6.8.0
rm -rf /usr/local/oeAware-tmp/
cd $script_dir/
rm -rf bpftool
git clone https://github.com/libbpf/bpftool
cd bpftool/
git checkout v6.8.0
git submodule update --init
cd ./src/
make
make install prefix=/usr/local/oeAware-tmp/

# 生成 vmlinux.h (跳过，用openEuler的vmlinux.h，kylin的缺少 BPF_ANY)
# cd $script_dir/
# /usr/local/oeAware-tmp/sbin/bpftool btf dump file /sys/kernel/btf/vmlinux format c > /usr/local/oeAware-tmp/vmlinux.h

# 配置环境变量并构建oeaware
cd $script_dir/
export OEAWARE_LIBBPFTOOL_PATH=/usr/local/oeAware-tmp/sbin/
# export OEAWARE_VMLINUX_INC=/usr/local/oeAware-tmp/

rm -rf ${name}-${version}
mkdir ${name}-${version}
rsync  -r --exclude={'rpm','build','docs'} ../../../* ./${name}-${version}
tar cf ${name}-${version}.tar.gz ${name}-${version}/
rm -rf /root/rpmbuild/SOURCES/${name}-${version}.tar.gz
rsync ${name}-${version}.tar.gz /root/rpmbuild/SOURCES/${name}-${version}.tar.gz
QA_SKIP_RPATHS=1 rpmbuild -ba $spec_file
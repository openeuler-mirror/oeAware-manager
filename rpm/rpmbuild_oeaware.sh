#!/bin/bash
set -x
yum install -y git rsync
yum install -y rpmdevtools

spec_file="oeAware.spec"
script_dir=$(cd $(dirname $0);pwd)
commit_id=$(git rev-parse HEAD)
cd $script_dir/
version=$commit_id
name="oeAware-manager"

yum-builddep $spec_file
rpmdev-setuptree

rm -rf ${name}-${version}
mkdir ${name}-${version}
rsync  -r --exclude={'rpm','build','docs'} ../* ./${name}-${version}
tar cf ${name}-${version}.tar.gz ${name}-${version}/
rm -rf /root/rpmbuild/SOURCES/${name}-${version}.tar.gz
rsync ${name}-${version}.tar.gz /root/rpmbuild/SOURCES/${name}-${version}.tar.gz
QA_SKIP_RPATHS=1 rpmbuild --define "commit_id $commit_id" -ba $spec_file
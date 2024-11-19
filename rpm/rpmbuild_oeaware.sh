#!/bin/bash
set -x
spec_file="oeAware.spec"
script_dir=$(cd $(dirname $0);pwd)
cd $script_dir/
version=$(awk '/Version:/{print $2}' "$spec_file")

rm -rf oeAware-buildbysrc-${version}
mkdir oeAware-buildbysrc-${version}
rsync  -r --exclude={'rpm','build','tests','docs'} ../* ./oeAware-buildbysrc-${version}
tar cf oeAware-buildbysrc-${version}.tar.gz oeAware-buildbysrc-${version}/
rm -rf /root/rpmbuild/SOURCES/oeAware-buildbysrc-${version}.tar.gz
rsync oeAware-buildbysrc-${version}.tar.gz /root/rpmbuild/SOURCES/oeAware-buildbysrc-${version}.tar.gz
rpmbuild -ba $spec_file
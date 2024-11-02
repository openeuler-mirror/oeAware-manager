
# yum install log4cplus log4cplus-devel curl-devel  cmake make gcc-c++  boost-devel  yaml-cpp-devel gtest-devel gmock-devel kernel-devel libnl3 libnl3-devel
set -x
script_dir=$(cd $(dirname $0);pwd)
cd $script_dir/
os_arch=$(uname -m)
libkperf_version="v1.2.1"

mkdir build
cd build
if [[ "$os_arch" == "x86_64" ]]; then
    echo "[NOTE] x86_64 not support libkperf, so skip build libkperf!!!"
elif [ -f $(pwd)/libkperf/output/lib/libkperf.so ]; then
    echo "[NOTE] libkperf already exists, so skip build libkperf!!!"
else
    git clone --recurse-submodules https://gitee.com/openeuler/libkperf.git
    cd libkperf
    git checkout $libkperf_version
    sh build.sh
    cd ..
fi
libkperf_inc=$(pwd)/libkperf/output/include
libkperf_lib=$(pwd)/libkperf/output/lib

cmake .. -DLIB_KPERF_LIBPATH=${libkperf_lib} -DLIB_KPERF_INCPATH=${libkperf_inc}
make -j$(nproc)
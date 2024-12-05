
# yum install log4cplus log4cplus-devel curl-devel  cmake make gcc-c++  boost-devel  yaml-cpp-devel gtest-devel gmock-devel kernel-devel libnl3 libnl3-devel
#set -x
script_dir=$(cd $(dirname $0);pwd)
cd $script_dir/
os_arch=$(uname -m)
libkperf_version="v1.2.1" # only for build_kperf_by_src=ON
build_kperf_by_src="ON"

function usage() {
    echo ""
    echo "usage: build.sh [OPTIONS] [ARGS]"
    echo ""
    echo "The most commonly used build.sh options are:"
    echo "  -k |--kperfrpm     not build with libkperf by source code"
    echo "  -h |--help         show usage"
}

options=$(getopt -o kh --long kperfrpm,help -- "$@")
eval set -- "$options"
while true; do
    case "$1" in
        -k|--kperfrpm)
            build_kperf_by_src="OFF"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Error: Unknown option $1"
            exit 1
            ;;
    esac
done

mkdir build
cd build
libkperf_inc=$(pwd)/libkperf/output/include
libkperf_lib=$(pwd)/libkperf/output/lib
if [[ "$os_arch" == "x86_64" ]]; then
    echo "[NOTE] x86_64 not support libkperf, so skip build libkperf!!!"
elif [ -f $(pwd)/libkperf/output/lib/libkperf.so ]; then
    echo "[NOTE] libkperf already exists, so skip build libkperf!!!"
elif [[ "$os_arch" == "aarch64" && "$build_kperf_by_src" == "ON" ]]; then
    git clone --recurse-submodules https://gitee.com/openeuler/libkperf.git
    cd libkperf
    git checkout $libkperf_version
    sh build.sh
    cd ..
    mkdir ${script_dir}/include/oeaware/data/libkperf
    cp ${libkperf_inc}/* ${script_dir}/include/oeaware/data/libkperf
elif [[ "$os_arch" == "aarch64" && "$build_kperf_by_src" == "OFF" ]]; then
    echo "[NOTE] use libkperf by rpm"
    libkperf_inc=/usr/include/libkperf
    libkperf_lib=/usr/lib64/
fi


cmake .. -DLIB_KPERF_LIBPATH=${libkperf_lib} -DLIB_KPERF_INCPATH=${script_dir}/include/oeaware/data
make -j$(nproc)
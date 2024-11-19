
## how to build
```
cd analysis
git clone --recurse-submodules https://gitee.com/openeuler/libkperf.git
cd libkperf
git checkout v1.2.1
sh build.sh
cd ..

mkdir build
cd build
cmake .. -DLIB_KPERF_LIBPATH=$(pwd)/../libkperf/output/lib -DLIB_KPERF_INCPATH=$(pwd)/../libkperf/include/ -DCMAKE_VERBOSE_MAKEFILE=true
```
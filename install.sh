# install server,client,header
mkdir -p /usr/include/oeaware
cp -r build/output/include/* /usr/include/
cp -r build/output/bin/* /bin/

# install client,server config
mkdir -p /etc/oeAware
chmod 640 etc/*.yaml
cp etc/*.yaml /etc/oeAware/

# install sdk
cp build/output/sdk/liboeaware-sdk.so /lib64

#install plugin so
mkdir -p /lib64/oeAware-plugin
chmod 440 build/output/plugin/lib/*.so
cp build/output/plugin/lib/*.so /lib64/oeAware-plugin
# install config
mkdir -p /etc/oeAware/plugin
chmod 640 etc/plugin/*
cp etc/plugin/* /etc/oeAware/plugin
# install ko
chmod 400 build/output/plugin/ko/*.ko
mkdir -p /usr/lib/smc/
cp build/output/plugin/ko/smc_acc.ko /usr/lib/smc/
chcon -t modules_object_t /usr/lib/smc/smc_acc.ko >/dev/null 2>&1

# install hwprobe_tune
mkdir -p /etc/oeAware/hwprobe_tune/src
mkdir -p /etc/oeAware/hwprobe_tune/lib
cp src/plugin/tune/hwprobe/ext/hwprobe_ext_zbb_tune/src/zbb_tune.c /etc/oeAware/hwprobe_tune/src
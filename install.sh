mkdir -p /usr/include/oeaware
cp -r build/output/include/* /usr/include/oeaware
cp -r build/output/bin/* /bin/
mkdir -p /etc/oeaware
cp config.yaml /etc/oeaware
cp build/output/sdk/liboeaware-sdk.so /lib64

chmod 440 build/output/plugin/lib/*.so
cp build/output/plugin/lib/*.so /lib64/oeAware-plugin
chmod 640 build/output/plugin/lib/*.conf
cp build/output/plugin/lib/*.conf /lib64/oeAware-plugin
chmod 400 build/output/plugin/ko/*.ko
cp build/output/plugin/ko/smc_acc.ko /usr/lib/smc/smc_acc.ko
insmod /usr/lib/smc/smc_acc.ko
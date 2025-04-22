mkdir -p /usr/include/oeaware
cp -r build/output/include/* /usr/include/
cp -r build/output/bin/* /bin/
mkdir -p /etc/oeAware
chmod 640 ./config.yaml
cp config.yaml /etc/oeAware
chmod 640 ./preload.yaml
cp preload.yaml /etc/oeAware
chmod 640 ./info_cmd.yaml
cp info_cmd.yaml /etc/oeAware
chmod 640 ./etc/analysis_config.yaml
cp etc/analysis_config.yaml /etc/oeAware
cp build/output/sdk/liboeaware-sdk.so /lib64

mkdir -p /etc/oeAware-plugin/
mkdir -p /lib64/oeAware-plugin
chmod 440 build/output/plugin/lib/*.so
cp build/output/plugin/lib/*.so /lib64/oeAware-plugin
chmod 640 build/output/plugin/lib/*.conf
cp build/output/plugin/lib/*.conf /lib64/oeAware-plugin
chmod 640 build/output/plugin/lib/*.yaml
cp build/output/plugin/lib/*.yaml /lib64/oeAware-plugin
chmod 640 ./config/hardirq_tune.conf
cp ./etc/hardirq_tune.conf /lib64/oeAware-plugin/
chmod 400 build/output/plugin/ko/*.ko
mkdir -p /usr/lib/smc/
cp build/output/plugin/ko/smc_acc.ko /usr/lib/smc/
insmod /usr/lib/smc/smc_acc.ko
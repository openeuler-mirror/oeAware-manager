mkdir -p /usr/include/oeaware
cp build/include/* /usr/include/oeaware
cp build/bin/* /bin/
mkdir -p /etc/oeaware
cp config.yaml /etc/oeaware
cp build/src/sdk/liboeaware-sdk.so /lib64

cp build/output/plugin/lib/* /lib64/oeAware-plugin
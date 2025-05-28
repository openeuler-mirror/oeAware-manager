#! /bin/bash
#remove header
rm -rf /usr/include/oeaware
#remove config
rm -rf /etc/oeAware
#remove plugin so
rm -rf /lib64/oeAware-plugin
#remove smc_acc.ko
rm -rf /usr/lib/smc/

#remove sdk so
rm -rf /lib64/liboeaware-sdk.so

#remove client,server
rm -rf /bin/oeaware
rm -rf /bin/oeawarectl
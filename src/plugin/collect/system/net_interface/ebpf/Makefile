include Makefile.arch

INSTALL ?= install
CLANG ?= clang
CC ?= gcc
BPFTOOL ?= bpftool
MKFLAGS = -D__BPF_TRACING__ -D__TARGET_ARCH_${SRCARCH}

all:
	$(CLANG) -O2 -g -Wall -target bpf -I. ${MKFLAGS} -c net_flow_kernel.c -o net_flow_kernel.o
	$(BPFTOOL) gen skeleton net_flow_kernel.o > net_flow.skel.h

clean:
	rm -f *.o
	rm -f *.skel.h

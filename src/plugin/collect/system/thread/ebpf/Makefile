include Makefile.arch

CLANG ?= clang

BPF_CFLAGS := -g -O2 -Wall -target bpf -D__TARGET_ARCH_$(SRCARCH) -I./ -I${OEAWARE_VMLINUX_INC}

all: thread_collector.bpf.o thread_collector.skel.h

thread_collector.bpf.o: thread_collector.bpf.c
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

thread_collector.skel.h: thread_collector.bpf.o
	${OEAWARE_LIBBPFTOOL_PATH}bpftool gen skeleton $< > $@

clean:
	rm -f thread_collector.bpf.o thread_collector.skel.h

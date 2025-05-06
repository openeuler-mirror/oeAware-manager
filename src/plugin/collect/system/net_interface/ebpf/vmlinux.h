#ifndef VMLINUX_MULTI_ARCH_H
#define VMLINUX_MULTI_ARCH_H

#if defined(__TARGET_ARCH_arm64)
#include "vmlinux_aarch64.h"
#elif defined(__TARGET_ARCH_x86)
#include "vmlinux_x86_64.h"
#elif defined(__TARGET_ARCH_riscv)
#include "vmlinux_riscv.h"
#endif

#endif
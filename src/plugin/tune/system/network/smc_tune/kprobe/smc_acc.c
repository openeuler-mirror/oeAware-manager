/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <linux/version.h>
#include <linux/sockptr.h>
#include <linux/net.h>

static struct kprobe kp_smc_connect = {
    .symbol_name = "smc_connect",
};

static struct kprobe kp_sys_connect_file = {
    .symbol_name = "__sys_connect_file"
};

static struct kprobe kp_smc_send = {
    .symbol_name = "smc_sendmsg",
};

static struct kprobe kp_smc_release = {
    .symbol_name = "smc_release",
};

static struct kprobe kp_sys_listen = {
    .symbol_name = "__sys_listen",
};

static struct kprobe kp_tcp_conn = {
    .symbol_name = "tcp_connect",
};

static struct kprobe kp_tcp_recv = {
    .symbol_name = "tcp_recvmsg",
};

static struct kprobe kp_tcp_fin = {
    .symbol_name = "tcp_fin",
};

#ifdef __x86_64__
#define REGS_PARM1(x) ((x)->di)
#define REGS_PARM2(x) ((x)->si)
#define REGS_PARM3(x) ((x)->dx)
#define REGS_PARM4(x) ((x)->cx)
#define REGS_PARM5(x) ((x)->r8)
#define REGS_PARM6(x) ((x)->r9)
#elif defined(__aarch64__)
#define REGS_PARM1(x) ((x)->regs[0])
#define REGS_PARM2(x) ((x)->regs[1])
#define REGS_PARM3(x) ((x)->regs[2])
#define REGS_PARM4(x) ((x)->regs[3])
#define REGS_PARM5(x) ((x)->regs[4])
#define REGS_PARM6(x) ((x)->regs[5])
#endif

#define SMCPROTO_SMC 0  /* SMC protocol, IPv4 */
#define SMCPROTO_SMC6 1 /* SMC protocol, IPv6 */
#define FDPUT_FPUT 1

#define AF_SMC 43

#define SMC_LOADED 0
#define SMC_UNLOADED 1
static int is_smc_loaded = SMC_LOADED;

#define VALID_VALUE 1
#define STR_LEN 512
static char blackPortList[STR_LEN] = "";
static char whitePortList[STR_LEN] = "";

module_param_string(black_port_list_param, blackPortList, sizeof(blackPortList), 0644);
MODULE_PARM_DESC(black_port_list_param, " A string parameter for filtering port not to switch TCP to SMC.\n"
                                        "       By default, accelerate all ports except those on the blacklist.\n"
                                        "       Separated by \',\' between ports. String length: 512 \n"
                                        "       e.g. black_port_list_param=\"80,8448\"");
static DEFINE_IDR(black_port_map);
module_param_string(white_port_list_param, whitePortList, sizeof(whitePortList), 0644);
MODULE_PARM_DESC(white_port_list_param, " A string parameter for filtering ports to switch TCP to SMC.\n "
                                        "       By default, accelerate all ports except those on the blacklist.\n"
                                        "       Separated by \',\' between ports. String length: 512.\n"
                                        "       e.g. white_port_list_param=\"80,8448\"");
static DEFINE_IDR(white_port_map);

static long long unsigned int conn_count_smc;
static long long unsigned int send_count_smc;
static long long unsigned int release_count_smc;
static long long unsigned int conn_count_tcp;
static long long unsigned int send_count_tcp;
static long long unsigned int release_count_tcp;
static int net_tcp2smc = 1;
#define NET_TCP2SMC_THRESHOLD 1024

#define REFRESH_COUNT(type)       \
    {                             \
        conn_count_##type = 0;    \
        send_count_##type = 0;    \
        release_count_##type = 0; \
    }

static unsigned long (*kallsyms_lookup_name_sym)(const char *name);
static int _kallsyms_lookup_kprobe(struct kprobe *p, struct pt_regs *regs)
{
    return 0;
}

static int parse_and_store_ports(const char *port_list, struct idr *port_map)
{
    char *token;
    int port, rc = 0;
    char *buf = (char *)port_list;  // 直接使用 blackPortList

    // 使用 strsep 分割字符串
    while ((token = strsep(&buf, ",")) != NULL) {
        if (strlen(token) == 0) {
            continue;
        }
        if (kstrtoint(token, 10, &port)) {
            pr_err("Failed to convert token to int: %s\n", token);
            rc = -EINVAL;
            break;
        }

        // 将 port 存储到 idr map
        if (idr_alloc(port_map, (void *)VALID_VALUE, port, port + 1, GFP_KERNEL) < 0) {
            pr_err("Failed to allocate idr for port: %d\n", port);
            rc = -ENOMEM;
            break;
        }
    }

    return rc;
}

static bool is_port_in_map(struct idr *port_map, int port)
{
    return idr_find(port_map, port) != NULL;
}

static bool check_port(int port)
{
    if (!idr_is_empty(&black_port_map))
        return !is_port_in_map(&black_port_map, port);

    if (!idr_is_empty(&white_port_map))
        return is_port_in_map(&white_port_map, port);
    return true;
}

static int handler_smc(int ifd)
{
    long ret;
    int tmperr;
    struct socket *sock;

    struct fd f = fdget(ifd);
    if (!f.file) {
        fdput(f);
        return 0;
    }

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 10, 0)
    sock = sock_from_file(f.file, &tmperr);
#else
    sock = sock_from_file(f.file);
#endif

    if (sock) {
        struct sock *sk = sock->sk;
        if (sk) {
            u16 protocol = sk->sk_protocol;
            u16 family = sk->sk_family;
            u16 type = sk->sk_type;
            if ((family == AF_INET || family == AF_INET6) && ((type & 0xf) == SOCK_STREAM) &&
                (protocol == IPPROTO_TCP || protocol == IPPROTO_IP)) {
                if (!check_port(sk->sk_num)) {
                    goto out;
                }
                ret = tcp_setsockopt(sk, SOL_TCP, TCP_ULP, KERNEL_SOCKPTR("smc"), sizeof("smc"));
                if (ret) {
                    printk(KERN_INFO "kprobe:  listen failed to set smc failed error id : %ld\n", ret);
                }
            }
        }
    }
out:
    fdput(f);
    return 0;
}

static int __kprobes handler_sys_listen(struct kprobe *p, struct pt_regs *regs)
{
    int ifd = REGS_PARM1(regs);

    if (!p) {
        return 0;
    }

    handler_smc(ifd);
    return 0;
}

static unsigned long get_kallsyms_func(char *func_name)
{
    struct kprobe probe;
    int ret;
    unsigned long addr;

    memset(&probe, 0, sizeof(probe));

    probe.pre_handler = _kallsyms_lookup_kprobe;
    probe.symbol_name = func_name;
    ret = register_kprobe(&probe);
    if (ret)
        return 0;
    addr = (unsigned long)probe.addr;
    unregister_kprobe(&probe);
    return addr;
}

static unsigned long generic_kallsyms_lookup_name(const char *name)
{
    if (!kallsyms_lookup_name_sym) {
        kallsyms_lookup_name_sym = (void *)get_kallsyms_func("kallsyms_lookup_name");
        if (!kallsyms_lookup_name_sym)
            return 0;
    }
    return kallsyms_lookup_name_sym(name);
}

static int __init check_smc_module(void)
{
    if (generic_kallsyms_lookup_name("smc_ism_init") == 0) {
        printk(KERN_ERR "SMC module is not loaded.\n");
        return 1;
    }
    return 0;
}

static int __kprobes handle_smc_connect(struct kprobe *p, struct pt_regs *regs)
{
    if (!net_tcp2smc) {
        return 0;
    }

    conn_count_smc += 1;
    return 0;
}

static int __kprobes handle_smc_sendmsg(struct kprobe *p, struct pt_regs *regs)
{
    if (!net_tcp2smc) {
        return 0;
    }

    send_count_smc += 1;

    return 0;
}

static int __kprobes handle_smc_release(struct kprobe *p, struct pt_regs *regs)
{
    if (!net_tcp2smc) {
        return 0;
    }

    release_count_smc += 1;
    if (conn_count_smc > NET_TCP2SMC_THRESHOLD && release_count_smc >= NET_TCP2SMC_THRESHOLD) {
        long long unsigned int real_send_count = send_count_smc / 2;
        if (conn_count_smc * 80 > real_send_count) {
            printk(KERN_INFO "handle_smc_release : close net_tcp2smc conn_count_smc %llu real_send_count %llu\n",
                conn_count_smc,
                send_count_smc);
            net_tcp2smc = 0;
        }
        REFRESH_COUNT(smc);
    }
    return 0;
}

static int __kprobes handle_tcp_conn(struct kprobe *p, struct pt_regs *regs)
{
    if (net_tcp2smc) {
        return 0;
    }

    conn_count_tcp += 1;
    return 0;
}

static int __kprobes handle_tcp_recv(struct kprobe *p, struct pt_regs *regs)
{
    if (net_tcp2smc) {
        return 0;
    }

    send_count_tcp += 1;
    return 0;
}

static int __kprobes handle_tcp_fin(struct kprobe *p, struct pt_regs *regs)
{
    if (net_tcp2smc) {
        return 0;
    }

    release_count_tcp += 1;
    if (conn_count_tcp > NET_TCP2SMC_THRESHOLD && release_count_tcp >= NET_TCP2SMC_THRESHOLD) {
        long long unsigned int real_send_count = send_count_tcp / 2;

        if (conn_count_tcp * 80 < real_send_count) {
            printk(KERN_INFO "handle_tcp_fin : open net_tcp2smc conn_count %llu real_send_count %llu\n",
                conn_count_tcp,
                real_send_count);
            net_tcp2smc = 1;
        }
        REFRESH_COUNT(tcp);
    }
    return 0;
}

static int __kprobes handler_connect_file(struct kprobe *p, struct pt_regs *regs)
{
    if (!net_tcp2smc) {
        return 0;
    }
    struct socket *sock;
    int tmperr;
    u16 dst_port;
    struct file *file = REGS_PARM1(regs);
    if (file == NULL) {
        return 0;
    }

    struct sockaddr_storage *address = REGS_PARM2(regs);
    if (address == NULL) {
        return 0;
    }

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 10, 0)
    sock = sock_from_file(file, &tmperr);
#else
    sock = sock_from_file(file);
#endif
    if (sock) {
        struct sock *sk = sock->sk;
        if (sk) {
            u16 protocol = sk->sk_protocol;
            u16 family = sk->sk_family;
            u16 type = sk->sk_type;
            if ((family == AF_INET || family == AF_INET6) && ((type & 0xf) == SOCK_STREAM) &&
                (protocol == IPPROTO_TCP || protocol == IPPROTO_IP)) {
                if (address->ss_family == AF_INET) {
                    struct sockaddr_in *addr_in = (struct sockaddr_in *)address;
                    dst_port = ntohs(addr_in->sin_port);
                } else if (address->ss_family == AF_INET6) {
                    struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)address;
                    dst_port = ntohs(addr_in6->sin6_port);
                }
                if (!check_port(dst_port)) {
                    return 0;
                }
                int ret = tcp_setsockopt(sk, SOL_TCP, TCP_ULP, KERNEL_SOCKPTR("smc"), sizeof("smc"));
                if (ret) {
                    printk(KERN_INFO "kprobe: connect failed to set smc failed error id : %ld\n", ret);
                }
            }
        }
    }
    return 0;
}

static int __init kprobe_init(void)
{
    int rc = 0;
    if (check_smc_module() != 0) {
        is_smc_loaded = SMC_UNLOADED;
        rc = -ENOENT;
        goto out;
    }
    rc = parse_and_store_ports(blackPortList, &black_port_map);
    if (rc != 0) {
        goto out;
    }
    rc = parse_and_store_ports(whitePortList, &white_port_map);
    if (rc != 0) {
        goto out;
    }
    kp_sys_listen.pre_handler = handler_sys_listen;
    kp_sys_connect_file.pre_handler = handler_connect_file;
    kp_smc_connect.pre_handler = handle_smc_connect;
    kp_smc_send.pre_handler = handle_smc_sendmsg;
    kp_smc_release.pre_handler = handle_smc_release;
    kp_tcp_conn.pre_handler = handle_tcp_conn;
    kp_tcp_fin.pre_handler = handle_tcp_fin;
    kp_tcp_recv.pre_handler = handle_tcp_recv;

    register_kprobe(&kp_sys_listen);
    register_kprobe(&kp_sys_connect_file);
    register_kprobe(&kp_smc_connect);
    register_kprobe(&kp_smc_send);
    register_kprobe(&kp_smc_release);
    register_kprobe(&kp_tcp_conn);
    register_kprobe(&kp_tcp_fin);
    register_kprobe(&kp_tcp_recv);
    printk(KERN_INFO "smc_acc : module loaded\n");
    return rc;
out:
    idr_destroy(&white_port_map);
    idr_destroy(&black_port_map);
    return rc;
}

static void __exit kprobe_exit(void)
{
    if (is_smc_loaded == SMC_UNLOADED) {
        goto out;
    }

    unregister_kprobe(&kp_sys_listen);
    unregister_kprobe(&kp_sys_connect_file);
    unregister_kprobe(&kp_smc_connect);
    unregister_kprobe(&kp_smc_send);
    unregister_kprobe(&kp_smc_release);
    unregister_kprobe(&kp_tcp_conn);
    unregister_kprobe(&kp_tcp_fin);
    unregister_kprobe(&kp_tcp_recv);
out:
    idr_destroy(&black_port_map);
    idr_destroy(&white_port_map);
    printk(KERN_INFO "smc_acc : module unloaded\n");
    return;
}

module_init(kprobe_init);
module_exit(kprobe_exit);
MODULE_LICENSE("GPL");
/******************************************************************************
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_endian.h>

#define ETH_P_IP   0x0800  // IPv4以太网协议类型
#define TC_ACT_OK  0       // TC放行动作

#define LOOPBACK_IP 0x0100007F // 127.0.0.1

struct SockKey {
    uint32_t localIp;
    uint32_t remoteIp;
    uint16_t localPort;
    uint16_t remotePort;
};

struct ThreadData {
    uint32_t pid;
    uint32_t tid;
    char comm[16];
};

struct SockInfo {
    struct ThreadData client;
    struct ThreadData server;
    uint64_t flow;
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, struct SockKey);
    __type(value, struct SockInfo);
} flowStats SEC(".maps");

static __always_inline void UpdateThreadData(struct ThreadData *td)
{
    td->pid = bpf_get_current_pid_tgid() >> 32;
    td->tid = (u32)bpf_get_current_pid_tgid();
    bpf_get_current_comm(&td->comm, sizeof(td->comm));
}

static __always_inline void ReadIpAndPort(struct sock *sk, struct SockKey *key)
{
    BPF_CORE_READ_INTO(&(key->localIp), sk, __sk_common.skc_rcv_saddr);
    BPF_CORE_READ_INTO(&(key->remoteIp), sk, __sk_common.skc_daddr);
    BPF_CORE_READ_INTO(&(key->localPort), sk, __sk_common.skc_num);
    BPF_CORE_READ_INTO(&(key->remotePort), sk, __sk_common.skc_dport);
}

SEC("kprobe/tcp_connect")
int BPF_KPROBE(tcp_connect, struct sock *sk)
{
    struct SockKey key;
    ReadIpAndPort(sk, &key);
    key.localPort = bpf_ntohs(key.localPort);
    key.remotePort = bpf_ntohs(key.remotePort);
    // check loopback
    struct SockInfo info = {};
    UpdateThreadData(&info.client);
    bpf_map_update_elem(&flowStats, &key, &info, BPF_ANY);
    return 0;
}

SEC("kretprobe/inet_csk_accept")
int BPF_KRETPROBE(inet_csk_accept_exit, struct sock *sk)
{
    struct SockKey tmp;
    ReadIpAndPort(sk, &tmp);
    // check loopback
    struct SockKey key = {
            .localIp = tmp.remoteIp,
            .remoteIp = tmp.localIp,
            .localPort = tmp.remotePort,
            .remotePort = tmp.localPort
    };
    struct SockInfo *info = bpf_map_lookup_elem(&flowStats, &key);
    if (info) {
        UpdateThreadData(&info->server);
    }
    return 0;
}

SEC("kprobe/tcp_close")
int BPF_KPROBE(tcp_close, struct sock *sk)
{
    struct SockKey key;
    ReadIpAndPort(sk,  &key);
    key.localPort = bpf_ntohs(key.localPort);
    key.remotePort = bpf_ntohs(key.remotePort);
    bpf_map_delete_elem(&flowStats, &key);
    return 0;
}

SEC("tc")
int tc_ingress(struct __sk_buff *skb) {
    void *dataEnd = (void *)(long)skb->data_end;
    void *data = (void *)(long)skb->data;

    // 解析以太网头
    struct ethhdr *eth = data;
    if (data + sizeof(*eth) > dataEnd)
        return TC_ACT_OK;

    // 仅处理IPv4
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return TC_ACT_OK;

    // 解析IP头
    struct iphdr *ip = data + sizeof(*eth);
    if (data + sizeof(*eth) + sizeof(*ip) > dataEnd)
        return TC_ACT_OK;

    // 仅处理TCP/UDP
    if (ip->protocol != IPPROTO_TCP && ip->protocol != IPPROTO_UDP)
        return TC_ACT_OK;

    // 解析传输层头
    __u16 hdr_len = ip->ihl * 4;
    struct tcphdr *tcp = (void *)ip + hdr_len;
    __u16 sport, dport;

    if (ip->protocol == IPPROTO_TCP) {
        if ((void *)tcp + sizeof(*tcp) > dataEnd)
            return TC_ACT_OK;
        sport = tcp->source;
        dport = bpf_ntohs(tcp->dest);
    } else {
        return TC_ACT_OK;
    }

    struct SockKey key;
    key.localIp = ip->saddr;
    key.remoteIp = ip->daddr;
    key.localPort = sport;
    key.remotePort = dport;
    struct SockInfo *info = bpf_map_lookup_elem(&flowStats, &key);
    if (info) {
        info->flow += skb->len;
    }
    return TC_ACT_OK;
}

char _license[] SEC("license") = "GPL";


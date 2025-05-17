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

static __always_inline void ReadIpAndPort(struct sock *sk, struct SockKey *key, struct SockKey *keyReversed)
{
    BPF_CORE_READ_INTO(&(key->localIp), sk, __sk_common.skc_rcv_saddr);
    BPF_CORE_READ_INTO(&(key->remoteIp), sk, __sk_common.skc_daddr);
    BPF_CORE_READ_INTO(&(key->localPort), sk, __sk_common.skc_num);
    BPF_CORE_READ_INTO(&(key->remotePort), sk, __sk_common.skc_dport);
    /*
    * accept/connect by socket, src port is host byte order, not need to convert
    * dport is network byte order, need to convert
    */
    key->remotePort = bpf_ntohs(key->remotePort);
    if (keyReversed) {
        keyReversed->localIp = key->remoteIp;
        keyReversed->remoteIp = key->localIp;
        keyReversed->localPort = key->remotePort;
        keyReversed->remotePort = key->localPort;
    }
}

SEC("kprobe/tcp_connect")
int BPF_KPROBE(tcp_connect, struct sock *sk)
{
    struct SockKey key;
    ReadIpAndPort(sk, &key, NULL);
    // check loopback
    struct SockInfo info = {};
    UpdateThreadData(&info.client);
    bpf_map_update_elem(&flowStats, &key, &info, BPF_ANY);
    return 0;
}

SEC("kretprobe/inet_csk_accept")
int BPF_KRETPROBE(inet_csk_accept_exit, struct sock *sk)
{
    struct SockKey key, keyReversed;
    ReadIpAndPort(sk, &key, &keyReversed);
    struct SockInfo *info = bpf_map_lookup_elem(&flowStats, &keyReversed);
    if (info) {
        UpdateThreadData(&info->server);
    }
    return 0;
}

SEC("kprobe/tcp_close")
int BPF_KPROBE(tcp_close, struct sock *sk)
{
    struct SockKey key, keyReversed;
    ReadIpAndPort(sk, &key, &keyReversed);
    bpf_map_delete_elem(&flowStats, &key);
    bpf_map_delete_elem(&flowStats, &keyReversed);
    return 0;
}

SEC("kprobe/tcp_set_state")
int BPF_KPROBE(tcp_set_state, struct sock *sk, int state)
{
    struct SockKey key, keyReversed;
    ReadIpAndPort(sk, &key, &keyReversed);
    uint8_t oldState = 0;
    int newState = state;
    BPF_CORE_READ_INTO(&oldState, sk, __sk_common.skc_state);
    struct ThreadData td;
    UpdateThreadData(&td);

    if (oldState == TCP_SYN_SENT && newState == TCP_ESTABLISHED) {
        struct SockInfo info = {};
        UpdateThreadData(&info.client);
        bpf_map_update_elem(&flowStats, &key, &info, BPF_ANY);
    } else if (oldState == TCP_SYN_RECV && newState == TCP_ESTABLISHED) {
        // This is a soft interrupt that is likely to occur on the client thread and requires IP reversal
        struct SockInfo info = {};
        UpdateThreadData(&info.client);
        bpf_map_update_elem(&flowStats, &keyReversed, &info, BPF_ANY);
    } else if ((oldState == TCP_ESTABLISHED && newState == TCP_FIN_WAIT1) || newState == TCP_CLOSE) {
        bpf_map_delete_elem(&flowStats, &key);
        bpf_map_delete_elem(&flowStats, &keyReversed);
    }
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
        // read from network packet, sport and dport are network byte order, need to convert
        sport = bpf_ntohs(tcp->source);
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
    struct SockKey keyReversed = {
        .localIp = key.remoteIp,
        .remoteIp = key.localIp,
        .localPort = key.remotePort,
        .remotePort = key.localPort
    };
    info = bpf_map_lookup_elem(&flowStats, &keyReversed);
    if (info) {
        info->flow += skb->len;
    }
    return TC_ACT_OK;
}

char _license[] SEC("license") = "GPL";


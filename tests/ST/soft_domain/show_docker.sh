#!/bin/bash

# 打印docker的soft_domain参数
print_soft_domain_params() {
    local container_name=$1
    # 获取完整的容器ID（64位）
    local container_id=$(docker ps -aq --no-trunc --filter "name=^/${container_name}$" | head -n 1)
    
    if [ -z "${container_id}" ]; then
        echo "  ${container_name}: Container not found"
        return
    fi
    
    local cgroup_cpu_path="/sys/fs/cgroup/cpu/docker/${container_id}"
    local cgroup_cpuset_path="/sys/fs/cgroup/cpuset/docker/${container_id}"
    
    # soft_domain参数
    local nr_cpu_file="${cgroup_cpu_path}/cpu.soft_domain_nr_cpu"
    local domain_file="${cgroup_cpu_path}/cpu.soft_domain"
    
    local nr_cpu="N/A"
    local domain="N/A"
    
    if [ -f "${nr_cpu_file}" ]; then
        nr_cpu=$(cat "${nr_cpu_file}" 2>/dev/null | tr -d ' \n\r')
    fi
    
    if [ -f "${domain_file}" ]; then
        domain=$(cat "${domain_file}" 2>/dev/null | tr -d ' \n\r')
    fi
    
    # CPU配额（从cpu.cfs_quota_us和cpu.cfs_period_us计算）
    local cpu_quota="N/A"
    local quota_file="${cgroup_cpu_path}/cpu.cfs_quota_us"
    local period_file="${cgroup_cpu_path}/cpu.cfs_period_us"
    
    if [ -f "${quota_file}" ] && [ -f "${period_file}" ]; then
        local quota_val=$(cat "${quota_file}" 2>/dev/null | tr -d ' \n\r')
        local period_val=$(cat "${period_file}" 2>/dev/null | tr -d ' \n\r')
        
        if [ "${quota_val}" = "-1" ]; then
            cpu_quota="unlimited"
        elif [ -n "${quota_val}" ] && [ -n "${period_val}" ] && [ "${period_val}" != "0" ]; then
            # 计算CPU配额，保留1位小数
            cpu_quota=$(awk "BEGIN {printf \"%.1f\", ${quota_val} / ${period_val}}" 2>/dev/null)
        fi
    fi
    
    # CPU绑定（cpuset.cpus）
    local cpuset_cpus="N/A"
    local cpuset_file="${cgroup_cpuset_path}/cpuset.cpus"
    
    if [ -f "${cpuset_file}" ]; then
        cpuset_cpus=$(cat "${cpuset_file}" 2>/dev/null | tr -d ' \n\r')
    fi
    
    # NUMA绑定（cpuset.mems）
    local numa_mems="N/A"
    local mems_file="${cgroup_cpuset_path}/cpuset.mems"
    
    if [ -f "${mems_file}" ]; then
        numa_mems=$(cat "${mems_file}" 2>/dev/null | tr -d ' \n\r')
    fi
    
    echo "  ${container_name}: cpu_quota=${cpu_quota}, cpuset_cpus=${cpuset_cpus}, numa_mems=${numa_mems}, soft_domain_nr_cpu=${nr_cpu}, soft_domain=${domain}"
}

# 获取所有运行中的docker容器名称
get_all_docker_names() {
    docker ps --format "{{.Names}}" | sort
}

echo "Starting docker monitoring (Press Ctrl+C to exit)..."
echo ""

# 捕获Ctrl+C信号，优雅退出
trap 'echo ""; echo "Monitoring stopped."; exit 0' INT

# 持续监控所有docker容器
while true; do
    echo "=========================================="
    echo "Soft domain parameters ($(date '+%Y-%m-%d %H:%M:%S')):"
    echo "=========================================="
    
    # 获取所有docker容器名称
    docker_names=$(get_all_docker_names)
    
    if [ -z "${docker_names}" ]; then
        echo "  No running docker containers found"
    else
        # 遍历所有docker容器
        while IFS= read -r container_name; do
            if [ -n "${container_name}" ]; then
                print_soft_domain_params "${container_name}"
            fi
        done <<< "${docker_names}"
    fi
    
    echo ""
    
    # 等待2秒
    sleep 2
done


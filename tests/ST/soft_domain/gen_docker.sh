#!/bin/bash

if [ $# -ne 4 ]; then
    echo "Usage: $0 <cpu> <bind> <num> <name>"
    echo "  cpu:  docker CPU quota, 0 means no quota limit"
    echo "  bind: CPU binding parameter, 0 means no binding, >0 means bind CPUs"
    echo "  num:  number of docker containers to create"
    echo "  name: docker name prefix"
    exit 1
fi

CPU=$1
BIND=$2
NUM=$3
NAME=$4

# 获取NUMA节点数量
NUMA_NUM=$(numactl --hardware | grep "available:" | awk '{print $2}')
if [ -z "$NUMA_NUM" ]; then
    echo "Error: Failed to get NUMA node count"
    exit 1
fi

# 计算每个NUMA分配的docker数量
DOCKERS_PER_NUMA=$((NUM / NUMA_NUM))
if [ $DOCKERS_PER_NUMA -eq 0 ]; then
    DOCKERS_PER_NUMA=1
fi

# 获取指定NUMA节点的所有CPU列表
get_numa_cpus() {
    local numa_id=$1
    local cpulist_file="/sys/devices/system/node/node${numa_id}/cpulist"
    
    if [ -f "${cpulist_file}" ]; then
        cat "${cpulist_file}" 2>/dev/null | tr -d ' \n\r'
    else
        echo ""
    fi
}

# 下载并导入openEuler docker镜像
IMAGE_FILE="openEuler-docker.aarch64.tar.xz"
IMAGE_URL="https://mirrors.aliyun.com/openeuler/openEuler-22.03-LTS-SP4/docker_img/aarch64/${IMAGE_FILE}"

# 检查是否已有openEuler镜像
IMAGE_NAME=$(docker images --format "{{.Repository}}:{{.Tag}}" | grep -i openeuler | head -n 1)

if [ -z "${IMAGE_NAME}" ]; then
    echo "Downloading openEuler docker image..."
    if [ ! -f "${IMAGE_FILE}" ]; then
        wget -q "${IMAGE_URL}"
        if [ $? -ne 0 ]; then
            echo "Error: Failed to download docker image"
            exit 1
        fi
    fi
    
    echo "Loading docker image..."
    LOAD_OUTPUT=$(docker load -i "${IMAGE_FILE}" 2>&1)
    if [ $? -ne 0 ]; then
        echo "Error: Failed to load docker image"
        exit 1
    fi
    
    # 从加载输出中获取镜像名称
    IMAGE_NAME=$(echo "${LOAD_OUTPUT}" | grep "Loaded image" | sed 's/.*Loaded image: //' | head -n 1)
    
    # 如果无法获取，尝试从docker images获取
    if [ -z "${IMAGE_NAME}" ]; then
        IMAGE_NAME=$(docker images --format "{{.Repository}}:{{.Tag}}" | grep -i openeuler | head -n 1)
    fi
    
    if [ -z "${IMAGE_NAME}" ]; then
        echo "Error: Could not determine image name after loading"
        exit 1
    fi
    
    echo "Docker image loaded successfully: ${IMAGE_NAME}"
else
    echo "Using existing docker image: ${IMAGE_NAME}"
fi

# 编译load程序（如果不存在）
if [ ! -f ./load ]; then
    echo "Compiling load.cpp..."
    g++ -o load load.cpp -pthread -std=c++11
    if [ $? -ne 0 ]; then
        echo "Error: Failed to compile load.cpp"
        exit 1
    fi
fi


# 创建docker容器
for ((i=0; i<NUM; i++)); do
    NUMA_ID=$((i / DOCKERS_PER_NUMA))
    DOCKER_ID_IN_NUMA=$((i % DOCKERS_PER_NUMA))
    DOCKER_NAME="${NAME}_${i}"
    
    # 构建docker run命令
    DOCKER_CMD="docker run -d --name ${DOCKER_NAME}"
    
    # 设置CPU配额
    if [ $CPU -gt 0 ]; then
        DOCKER_CMD="${DOCKER_CMD} --cpus=${CPU}"
    fi
    
    # 设置CPU绑定
    if [ $BIND -gt 0 ]; then
        # 获取对应NUMA节点的所有CPU
        CPUSET=$(get_numa_cpus ${NUMA_ID})
        if [ -z "${CPUSET}" ]; then
            echo "Error: Failed to get CPUs for NUMA node ${NUMA_ID}"
            exit 1
        fi
        DOCKER_CMD="${DOCKER_CMD} --cpuset-cpus=${CPUSET}"
    fi
    
    # 挂载当前目录并运行load程序
    DOCKER_CMD="${DOCKER_CMD} -v $(pwd):/workspace -w /workspace ${IMAGE_NAME} sh -c './load 3 ${DOCKER_NAME}'"
    
    echo "Creating docker ${DOCKER_NAME}..."
    CONTAINER_ID=$(eval $DOCKER_CMD)
    
    if [ $? -ne 0 ] || [ -z "${CONTAINER_ID}" ]; then
        echo "Error: Failed to create docker ${DOCKER_NAME}"
        exit 1
    fi
    
    # 等待容器启动
    sleep 0.5
done

echo ""
echo "Successfully created ${NUM} docker containers with prefix ${NAME}"
echo ""
echo "Use './show_docker.sh' to monitor all docker containers"

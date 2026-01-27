#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <name>"
    echo "  name: docker name pattern (will delete all containers whose name contains this pattern)"
    exit 1
fi

NAME=$1

# 查找所有名字包含指定模式的容器
CONTAINER_IDS=$(docker ps -aq --filter "name=${NAME}")

if [ -z "${CONTAINER_IDS}" ]; then
    echo "No docker containers found with name pattern: ${NAME}"
    exit 0
fi

# 显示要删除的容器
echo "Found the following containers to delete:"
docker ps -a --filter "name=${NAME}" --format "  {{.Names}} ({{.ID}})"

# 确认删除
echo ""
read -p "Are you sure you want to delete these containers? (y/N): " confirm
if [ "${confirm}" != "y" ] && [ "${confirm}" != "Y" ]; then
    echo "Cancelled."
    exit 0
fi

# 删除容器
echo "Deleting containers..."
docker rm -f ${CONTAINER_IDS}

if [ $? -eq 0 ]; then
    echo "Successfully deleted all containers with name pattern: ${NAME}"
else
    echo "Error: Failed to delete some containers"
    exit 1
fi


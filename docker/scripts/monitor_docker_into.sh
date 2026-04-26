#!/usr/bin/env bash
# monitor_docker_into.sh - 进入监控系统开发容器

set -e

CONTAINER_NAME="linux_monitor"
CURRENT_USER="${USER:-root}"

echo "进入容器: ${CONTAINER_NAME}"
echo "用户: ${CURRENT_USER}"

# 检查容器是否在运行
if ! docker ps | grep -q "${CONTAINER_NAME}"; then
    echo "错误: 容器 ${CONTAINER_NAME} 未运行"
    echo "请先运行: ./docker/scripts/monitor_docker_run.sh"
    exit 1
fi

docker exec -it "${CONTAINER_NAME}" /bin/bash
#!/usr/bin/env bash
# monitor_docker_run.sh - 启动监控系统开发容器

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# 配置
CONTAINER_NAME="linux_monitor"
IMAGE_NAME="monitor-dev"
IMAGE_TAG="latest"
FULL_IMAGE_NAME="${IMAGE_NAME}:${IMAGE_TAG}"

# 显示设置
if [ -z "${DISPLAY}" ]; then
    DISPLAY_VALUE=":1"
else
    DISPLAY_VALUE="${DISPLAY}"
fi

# 用户信息
LOCAL_HOST="$(hostname)"
CURRENT_USER="${USER}"
USER_ID="$(id -u)"
USER_GROUP="$(id -g -n)"
GROUP_ID="$(id -g)"

echo "========================================"
echo "启动监控系统开发容器"
echo "========================================"

# 检查镜像是否存在
echo "检查镜像..."
if ! docker images | grep -q "${IMAGE_NAME}.*${IMAGE_TAG}"; then
    echo "镜像 ${FULL_IMAGE_NAME} 不存在，正在构建..."
    
    # 检查 Dockerfile 是否存在
    if [ ! -f "$PROJECT_ROOT/docker/build/base.dockerfile" ]; then
        echo "错误: Dockerfile 不存在于 $PROJECT_ROOT/docker/build/"
        exit 1
    fi
    
    # 构建镜像
    cd "$PROJECT_ROOT/docker/build"
    docker build -f base.dockerfile -t "${FULL_IMAGE_NAME}" .
    
    if [ $? -ne 0 ]; then
        echo "镜像构建失败"
        exit 1
    fi
    
    echo "✅ 镜像构建完成"
else
    echo "✅ 镜像已存在: ${FULL_IMAGE_NAME}"
fi

# 停止并删除现有容器
echo "清理现有容器..."
if docker ps -a | grep -q "${CONTAINER_NAME}"; then
    echo "停止容器: ${CONTAINER_NAME}"
    docker stop "${CONTAINER_NAME}" > /dev/null 2>&1 || true
    
    echo "删除容器: ${CONTAINER_NAME}"
    docker rm -f "${CONTAINER_NAME}" > /dev/null 2>&1 || true
    
    echo "✅ 旧容器已清理"
else
    echo "没有正在运行的容器"
fi

# 创建必要目录
echo "准备挂载目录..."
mkdir -p "$PROJECT_ROOT/build"

# 启动新容器
echo "启动容器..."
docker run -it -d \
    --name "${CONTAINER_NAME}" \
    -e DISPLAY="${DISPLAY_VALUE}" \
    --privileged=true \
    -e DOCKER_USER="${CURRENT_USER}" \
    -e USER="${CURRENT_USER}" \
    -e DOCKER_USER_ID="${USER_ID}" \
    -e DOCKER_GRP="${USER_GROUP}" \
    -e DOCKER_GRP_ID="${GROUP_ID}" \
    -e XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR}" \
    -e QT_QPA_PLATFORM_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/qt5/plugins \
    -e QT_SELECT=qt5 \
    -v "${PROJECT_ROOT}:/work" \
    -v "${XDG_RUNTIME_DIR}:${XDG_RUNTIME_DIR}" \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v "$HOME/.Xauthority:/root/.Xauthority" \
    --net host \
    "${FULL_IMAGE_NAME}"

if [ $? -eq 0 ]; then
    echo "✅ 容器启动成功"
    echo ""
    echo "容器名称: ${CONTAINER_NAME}"
    echo "镜像: ${FULL_IMAGE_NAME}"
    echo "工作目录: /work"
    echo "显示: ${DISPLAY_VALUE}"
    echo ""
    echo "使用以下命令进入容器:"
    echo "  ./docker/scripts/monitor_docker_into.sh"
    echo "  或"
    echo "  docker exec -it ${CONTAINER_NAME} bash"
    echo ""
    echo "项目文件已挂载到容器的 /work 目录"
else
    echo "❌ 容器启动失败"
    exit 1
fi

# 等待容器完全启动
sleep 2

echo ""
echo "========================================"
echo "启动完成！"
echo "========================================"

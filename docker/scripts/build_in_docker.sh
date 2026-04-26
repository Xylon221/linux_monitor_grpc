#!/usr/bin/env bash
# build_in_docker.sh - 在容器内构建项目

set -e

CONTAINER_NAME="linux_monitor"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "在容器内构建监控系统..."

# 检查容器是否运行
if ! docker ps | grep -q "${CONTAINER_NAME}"; then
    echo "容器未运行，正在启动..."
    "$PROJECT_ROOT/docker/scripts/monitor_docker_run.sh"
fi

# 在容器内构建
docker exec "${CONTAINER_NAME}" bash -c "
    echo '=== 在容器内构建 ==='
    echo '工作目录: /work'
    echo '当前用户: \$(whoami)'
    
    cd /work
    
    # 清理并构建
    if [ -d 'build' ]; then
        echo '清理旧构建...'
        rm -rf build
    fi
    
    mkdir -p build
    cd build
    
    echo '运行 CMake...'
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    echo '编译项目...'
    make -j\$(nproc)
    
    echo ''
    echo '=== 构建结果 ==='
    
    # 检查生成的文件（正确路径）
    if [ -f 'linux_monitor/src/monitor' ]; then
        echo '✅ 监控客户端: linux_monitor/src/monitor'
        file linux_monitor/src/monitor
    elif [ -f '../linux_monitor/src/monitor' ]; then
        echo '✅ 监控客户端: ../linux_monitor/src/monitor'
        file ../linux_monitor/src/monitor
    else
        echo '❌ 监控客户端未生成'
    fi
    
    if [ -f 'bin/server' ]; then
        echo '✅ RPC服务器: bin/server'
        file bin/server
    else
        echo '❌ RPC服务器未生成'
    fi

    if [ -f 'display_monitor/display' ]; then
        echo '✅ 显示界面: display_monitor/display'
        file display_monitor/display
    elif [ -f '../display_monitor/display' ]; then
        echo '✅ 显示界面: ../display_monitor/display'
        file ../display_monitor/display
    else
        echo '❌ 显示界面未生成'
    fi
    
    echo ''
    echo '构建完成！'
    
    # 显示运行说明
    echo '=== 运行说明 ==='
    echo '1. 进入容器: ./docker/scripts/monitor_docker_into.sh'
    echo '2. 运行服务: cd /work/build && ./bin/server'
    echo '3. 运行监控: cd /work/build && ./linux_monitor/src/monitor'
    echo '4. 运行界面: cd /work/build && ./display_monitor/display'
"
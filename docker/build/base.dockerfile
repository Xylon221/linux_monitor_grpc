FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# 使用阿里云镜像源
COPY apt/sources.list /etc/apt/

# 分步安装依赖
RUN apt-get update && \
    # 安装基础包
    apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    git \
    wget \
    curl \
    pkg-config \
    autoconf \
    automake \
    libtool \
    python3 \
    python3-pip \
    python3-dev \
    && apt-get clean

# 安装 C++ 和系统库
RUN apt-get update && \
    apt-get install -y \
    libstdc++-12-dev \
    libboost-all-dev \
    libboost-chrono-dev \
    libboost-system-dev \
    libc-ares-dev \
    libssl-dev \
    libssl3 \
    && apt-get clean

# 安装 gRPC 和 Protobuf
RUN apt-get update && \
    apt-get install -y \
    libprotobuf-dev \
    protobuf-compiler \
    libgrpc++-dev \
    libgrpc-dev \
    protobuf-compiler-grpc \
    && apt-get clean

# 安装 Abseil
RUN apt-get update && \
    apt-get install -y \
    libabsl-dev \
    && apt-get clean

# 安装系统工具
RUN apt-get update && \
    apt-get install -y \
    vim \
    htop \
    net-tools \
    ninja-build \
    stress \
    && apt-get clean

# 安装 X11 显示支持
RUN apt-get update && \
    apt-get install -y \
    libx11-xcb1 \
    libfreetype6 \
    libdbus-1-3 \
    libfontconfig1 \
    libxkbcommon0 \
    libxkbcommon-x11-0 \
    libgl1-mesa-glx \
    libegl1-mesa \
    && apt-get clean

# 清理缓存
RUN rm -rf /var/lib/apt/lists/*

# 验证安装
RUN echo "=== 验证安装 ===" && \
    echo "CMake: $(cmake --version | head -n1)" && \
    echo "g++: $(g++ --version | head -n1)" && \
    echo "Qt: $(qmake --version | head -n1 || echo 'qmake not found')" && \
    echo "Protobuf: $(protoc --version || echo 'protoc not found')" && \
    echo "Python: $(python3 --version)"

WORKDIR /work

CMD ["/bin/bash"]
cmake_minimum_required(VERSION 3.10.2)
project(linux_monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(proto)

set(PROTO_BINARY_DIR ${CMAKE_BINARY_DIR}/proto)
include_directories(
    ${PROTO_BINARY_DIR}
    ${PROTO_SOURCE_DIR}  # 原始 proto 文件
)

add_subdirectory(rpc_manager)
add_subdirectory(linux_monitor)
add_dependencies(client_test monitor_proto)
add_dependencies(server monitor_proto)
add_dependencies(monitor monitor_proto)
cmake_minimum_required(VERSION 3.10.2)
project(monitor_proto)

# 设置变量给父级使用
set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
set(PROTO_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

# 查找工具
find_program(PROTOC_EXECUTABLE protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# proto 文件
set(PROTO_FILES
    monitor_info.proto
    cpu_load.proto
    cpu_softirq.proto
    cpu_stat.proto
    mem_info.proto
    net_info.proto
    ebpf_info.proto
)

# 生成所有文件
foreach(PROTO ${PROTO_FILES})
    # 生成普通 .pb.cc 和 .pb.h
    execute_process(
        COMMAND ${PROTOC_EXECUTABLE} --cpp_out=${CMAKE_CURRENT_BINARY_DIR} ${PROTO}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # 生成 gRPC .grpc.pb.cc 和 .grpc.pb.h
    execute_process(
        COMMAND ${PROTOC_EXECUTABLE}
                --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
                --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
                ${PROTO}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endforeach()

# 查找生成的文件
file(GLOB GENERATED_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/*.pb.cc ${CMAKE_CURRENT_BINARY_DIR}/*.grpc.pb.cc)
file(GLOB GENERATED_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/*.pb.h ${CMAKE_CURRENT_BINARY_DIR}/*.grpc.pb.h)

# 创建库
add_library(monitor_proto STATIC ${GENERATED_SOURCES})

# 使用 pkg-config 查找 gRPC 链接参数
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++ grpc protobuf gpr)

# 链接系统库
target_link_libraries(monitor_proto PUBLIC
    ${GRPC_LIBRARIES}
)

target_include_directories(monitor_proto PUBLIC
    ${GRPC_INCLUDE_DIRS}
)

target_link_directories(monitor_proto PUBLIC
    ${GRPC_LIBRARY_DIRS}
)

# 包含目录
target_include_directories(monitor_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)
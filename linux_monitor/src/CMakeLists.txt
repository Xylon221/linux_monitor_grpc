# eBPF/BCC and LLVM support
find_program(LLVM_CONFIG llvm-config)
if(LLVM_CONFIG)
    execute_process(COMMAND ${LLVM_CONFIG} --includedir
        OUTPUT_VARIABLE LLVM_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

set(SOURCES
    main.cpp
    monitor/cpu_softirq_monitor.cpp
    monitor/cpu_load_monitor.cpp
    monitor/cpu_stat_monitor.cpp
    monitor/mem_monitor.cpp
    monitor/net_monitor.cpp
    monitor/bpf_tcp_monitor.cpp
    utils/read_file.cpp
)

add_executable(monitor ${SOURCES})

# 包含目录配置
target_include_directories(monitor PUBLIC
    ${PROJECT_SOURCE_DIR}/linux_monitor/include
    ${PROJECT_SOURCE_DIR}/rpc_manager
    /usr/include/bcc
    /usr/include/bcc/compat
    ${LLVM_INCLUDE_DIR}
)

target_link_libraries(monitor
    PUBLIC
    monitor_proto
    client
    bcc
)

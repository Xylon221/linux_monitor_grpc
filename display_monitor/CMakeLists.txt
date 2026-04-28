# 移除 Qt 相关依赖
# find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# 移除 Qt 包含目录和库链接
# ${Qt5Widgets_INCLUDE_DIRS}
# ${Qt5Core_INCLUDE_DIRS}
# Qt5::Widgets
# Qt5::Core

# 保留其他配置
set(CMAKE_AUTOMOC OFF) # 关闭自动 MOC

# 列出所有源文件和头文件
set(SOURCES
    main.cpp
    # 如果还有其他.cpp文件，在这里添加
)

# 列出所有需要MOC的头文件
set(HEADERS
    cpu_load_model.h
    cpu_softirq_model.h
    cpu_stat_model.h
    mem_model.h
    monitor_inter.h
    monitor_widget.h
    net_model.h
)

# 创建可执行文件（包含源文件和头文件）
add_executable(display ${SOURCES} ${HEADERS})

# 包含目录设置
target_include_directories(display PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}       # 当前目录
    ${PROJECT_SOURCE_DIR}/rpc_manager # RPC客户端头文件
)

# 链接库设置
target_link_libraries(display
    client                 # RPC客户端库
    monitor_proto          # Protobuf库
)
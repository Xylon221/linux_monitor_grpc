#include <memory>
#include <thread>
#include <vector>
#include <cstdlib>

// 客户端RPC相关头文件
#include "client/rpc_client.h"            // RPC客户端实现

// 监控器头文件
#include "monitor/bpf_tcp_monitor.h"      // eBPF TCP连接监控
#include "monitor/cpu_load_monitor.h"     // CPU负载监控
#include "monitor/cpu_softirq_monitor.h"  // CPU软中断监控
#include "monitor/cpu_stat_monitor.h"     // CPU状态监控
#include "monitor/mem_monitor.h"          // 内存监控
#include "monitor/monitor_inter.h"        // 监控器接口基类
#include "monitor/net_monitor.h"          // 网络监控

// Protobuf生成的头文件
#include "monitor_info.grpc.pb.h"         // gRPC服务定义
#include "monitor_info.pb.h"              // Protobuf消息定义

/**
 * @brief 监控系统主程序入口
 *
 * 主程序功能概述：
 * 1. 创建并管理所有监控器实例
 * 2. 启动监控数据采集线程
 * 3. 定期（每3秒）采集系统各项指标
 * 4. 通过RPC客户端将数据发送到服务器
 *
 * 架构设计：
 * - 工厂模式：通过基类指针管理不同类型的监控器
 * - 策略模式：每个监控器实现特定的监控策略
 * - 观察者模式：定期观察系统状态变化
 * - 生产者-消费者模式：监控线程生产数据，RPC客户端消费数据
 *
 * 线程模型：
 * - 主线程：等待监控线程结束（实际是join，会阻塞）
 * - 监控线程：负责定期采集和发送数据
 *
 * 数据流：
 * 系统状态 → 监控器采集 → MonitorInfo消息 → RPC客户端 → 远程服务器
 */
int main(int argc, char* argv[])
{
    // ==================== 初始化监控器集合 ====================
    // 使用基类指针存储不同类型的监控器，实现多态
    std::vector<std::shared_ptr<monitor::MonitorInter>> runners_;

    // 添加各类监控器到集合中
    // 注意：使用new创建对象，由shared_ptr管理生命周期
    runners_.emplace_back(new monitor::CpuSoftIrqMonitor());    // CPU软中断监控
    runners_.emplace_back(new monitor::CpuLoadMonitor());       // CPU负载监控
    runners_.emplace_back(new monitor::CpuStatMonitor());       // CPU状态监控
    runners_.emplace_back(new monitor::MemMonitor());           // 内存监控
    runners_.emplace_back(new monitor::NetMonitor());           // 网络监控
    runners_.emplace_back(new monitor::BpfTcpMonitor());        // eBPF TCP连接监控

    // ==================== 初始化RPC客户端 ====================
    // 服务器地址优先级: 命令行参数 > 环境变量 GRPC_SERVER > 默认 localhost:50051
    std::string server_addr = "localhost:50051";
    if (argc > 1)
    {
        server_addr = argv[1];
    }
    else if (const char* env_addr = std::getenv("GRPC_SERVER"))
    {
        server_addr = env_addr;
    }
    monitor::RpcClient rpc_client_(server_addr);

    // ==================== 获取主机标识 ====================
    // 使用环境变量HOSTNAME作为主机名标识，优先于USER
    // 可通过 MONITOR_NAME 环境变量显式指定
    char* name = std::getenv("MONITOR_NAME");
    if (!name) name = std::getenv("HOSTNAME");
    if (!name) name = std::getenv("USER");

    // ==================== 启动监控线程 ====================
    std::unique_ptr<std::thread> thread_ = nullptr;

    // 使用lambda表达式创建监控线程
    thread_ = std::make_unique<std::thread>([&]() {
        // 线程主循环：持续监控和上报
        while (true)
        {
            // 创建监控数据消息
            monitor::proto::MonitorInfo monitor_info;

            // 设置主机标识
            if (name)
            {
                monitor_info.set_name(std::string(name));
            }
            else
            {
                monitor_info.set_name("unknown_host");  // 默认值
            }

            // 遍历所有监控器，采集数据
            for (auto& runner : runners_)
            {
                // 多态调用：每个监控器实现自己的UpdateOnce方法
                runner->UpdateOnce(&monitor_info);
            }

            // 通过RPC客户端发送监控数据
            rpc_client_.SetMonitorInfo(monitor_info);

            // 休眠3秒，控制监控频率
            // 注意：使用steady_clock确保休眠时间准确
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    });

    // ==================== 等待线程结束 ====================
    // 注意：由于监控线程是无限循环，这里会永久阻塞
    thread_->join();

    return 0;
}

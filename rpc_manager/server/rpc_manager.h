// 头文件保护宏，防止重复包含
#pragma once

// gRPC相关头文件
#include <grpcpp/support/status.h>
#include <grpcpp/server_context.h>

// C++标准库头文件
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <mutex>

// Protobuf和gRPC生成的头文件
#include "monitor_info.grpc.pb.h"
#include "monitor_info.pb.h"

namespace monitor
{
    /**
     * @brief gRPC服务实现类
     *
     * 继承自Protobuf生成的GrpcManager::Service基类
     * 实现多主机监控数据的接收和提供功能
     *
     * 多主机支持：
     * - SetMonitorInfo: 按主机名（request->name()）存储各客户端上报的数据
     * - GetMonitorInfo: 返回最新上报的主机数据（向后兼容）
     * - GetAllMonitorInfo: 返回所有主机的监控数据
     */
    class GrpcManagerImpl : public monitor::proto::GrpcManager::Service
    {
    public:
        /**
         * @brief 构造函数
         */
        GrpcManagerImpl() {}

        /**
         * @brief 析构函数
         */
        virtual ~GrpcManagerImpl() {}

        /**
         * @brief 设置监控信息RPC方法
         * @param context gRPC服务器上下文
         * @param request 客户端发送的监控信息
         * @param response 空响应
         * @return gRPC状态码
         *
         * 客户端调用此方法将监控数据发送到服务器
         * 按主机名（request->name()）存储，支持多主机并发上报
         */
        ::grpc::Status SetMonitorInfo(
            ::grpc::ServerContext* context,
            const ::monitor::proto::MonitorInfo* request,
            ::google::protobuf::Empty* response) override
        {
            std::lock_guard<std::mutex> lock(mutex_);

            const std::string& hostname = request->name();
            hostnames_.insert(hostname);
            monitor_infos_[hostname] = *request;
            latest_host_ = hostname;

            std::cout << "[SetMonitorInfo] host=" << hostname
                      << " soft_irq_size=" << request->soft_irq_size()
                      << " total_hosts=" << monitor_infos_.size() << std::endl;

            return grpc::Status::OK;
        }

        /**
         * @brief 获取监控信息RPC方法
         * @param context gRPC服务器上下文
         * @param request 空请求
         * @param response 服务器返回的监控信息
         * @return gRPC状态码
         *
         * 返回最新上报的主机数据（向后兼容单主机模式）
         */
        ::grpc::Status GetMonitorInfo(
            ::grpc::ServerContext* context,
            const ::google::protobuf::Empty* request,
            ::monitor::proto::MonitorInfo* response) override
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (!latest_host_.empty() && monitor_infos_.count(latest_host_))
            {
                *response = monitor_infos_[latest_host_];
            }

            return grpc::Status::OK;
        }

        /**
         * @brief 获取所有主机的监控信息RPC方法
         * @param context gRPC服务器上下文
         * @param request 空请求
         * @param response 所有主机的监控数据
         * @return gRPC状态码
         */
        ::grpc::Status GetAllMonitorInfo(
            ::grpc::ServerContext* context,
            const ::google::protobuf::Empty* request,
            ::monitor::proto::AllMonitorInfo* response) override
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto it = monitor_infos_.begin(); it != monitor_infos_.end(); ++it)
            {
                auto* host_info = response->add_hosts();
                *host_info = it->second;
            }

            return grpc::Status::OK;
        }

    private:
        /// @brief 存储所有主机的监控信息，key为hostname
        std::unordered_map<std::string, monitor::proto::MonitorInfo> monitor_infos_;

        /// @brief 跟踪所有已知主机名
        std::unordered_set<std::string> hostnames_;

        /// @brief 最新上报数据的主机名（用于向后兼容）
        std::string latest_host_;

        /// @brief 线程安全互斥锁
        std::mutex mutex_;
    };
}  // namespace monitor

#pragma once

#include <memory>
#include "monitor/monitor_inter.h"
#include "monitor_info.grpc.pb.h"
#include "monitor_info.pb.h"

namespace monitor
{
    class BpfTcpMonitor : public MonitorInter
    {
    public:
        BpfTcpMonitor();
        ~BpfTcpMonitor();
        void UpdateOnce(monitor::proto::MonitorInfo* monitor_info);
        void Stop() override;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}  // namespace monitor

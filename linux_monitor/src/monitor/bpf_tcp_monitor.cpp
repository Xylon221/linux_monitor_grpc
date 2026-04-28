#include "monitor/bpf_tcp_monitor.h"

#include <bcc/BPF.h>
#include <bcc/BPFTable.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace monitor
{
    namespace
    {
        // BPF C program: uses inet_sock_set_state tracepoint to track TCP state transitions.
        // Args provided by tracepoint: skaddr, oldstate, newstate, sport, dport, family, protocol
        const std::string BPF_PROGRAM = R"(
BPF_ARRAY(conn_state_count, u64, 13);
BPF_ARRAY(event_counter, u64, 1);
BPF_HASH(port_map, u32, u64);

TRACEPOINT_PROBE(sock, inet_sock_set_state)
{
    if (args->protocol != 6) return 0;

    u32 s = (u32)args->newstate;

    conn_state_count.increment(s);

    u32 key = 0;
    event_counter.increment(key);

    u16 sport = args->sport;
    if (sport > 0)
    {
        u32 pkey = (u32)sport;
        u64 *pcount = port_map.lookup(&pkey);
        if (pcount)
            (*pcount)++;
        else
        {
            u64 val = 1;
            port_map.update(&pkey, &val);
        }
    }

    return 0;
}
)";

        constexpr int EV_KEY = 0;

        constexpr int TCP_ESTABLISHED = 1;
        constexpr int TCP_SYN_SENT    = 2;
        constexpr int TCP_SYN_RECV    = 3;
        constexpr int TCP_FIN_WAIT1   = 4;
        constexpr int TCP_FIN_WAIT2   = 5;
        constexpr int TCP_TIME_WAIT   = 6;
        constexpr int TCP_CLOSE       = 7;
        constexpr int TCP_CLOSE_WAIT  = 8;
        constexpr int TCP_LAST_ACK    = 9;
        constexpr int TCP_LISTEN      = 10;
        constexpr int TCP_CLOSING     = 11;
    }  // anonymous namespace

    class BpfTcpMonitor::Impl
    {
    public:
        Impl()
            : bpf_ready_(false),
              last_events_(0),
              last_state_counts_(13, 0),
              last_time_(std::chrono::steady_clock::now())
        {
            try
            {
                auto ret = bpf_.init(BPF_PROGRAM);
                if (!ret.ok())
                {
                    std::cerr << "[BpfTcpMonitor] BPF init failed: "
                              << ret.msg() << std::endl;
                    return;
                }

                ret = bpf_.attach_tracepoint("sock:inet_sock_set_state",
                                             "tracepoint__sock__inet_sock_set_state");
                if (!ret.ok())
                {
                    std::cerr << "[BpfTcpMonitor] attach tracepoint failed: "
                              << ret.msg() << std::endl;
                    return;
                }

                bpf_ready_ = true;
                std::cout << "[BpfTcpMonitor] eBPF loaded: inet_sock_set_state tracepoint"
                          << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << "[BpfTcpMonitor] Exception: " << e.what() << std::endl;
            }
        }

        ~Impl() = default;

        void UpdateOnce(monitor::proto::MonitorInfo* monitor_info)
        {
            if (!bpf_ready_)
                return;

            try
            {
                auto now = std::chrono::steady_clock::now();
                double elapsed = std::chrono::duration<double>(now - last_time_).count();
                if (elapsed < 0.001) elapsed = 3.0;

                auto state_tbl = bpf_.get_array_table<uint64_t>("conn_state_count");
                std::vector<uint64_t> cur(13, 0);
                for (int i = 1; i <= 12; i++)
                    state_tbl.get_value(i, cur[i]);

                auto event_tbl = bpf_.get_array_table<uint64_t>("event_counter");
                uint64_t ev = 0;
                event_tbl.get_value(EV_KEY, ev);

                uint64_t ev_delta = ev - last_events_;
                float rate = (elapsed > 0) ? static_cast<float>(ev_delta / elapsed) : 0.0f;

                auto port_tbl = bpf_.get_hash_table<uint32_t, uint64_t>("port_map");
                auto port_entries = port_tbl.get_table_offline();
                for (auto& entry : port_entries)
                    port_tbl.remove_value(entry.first);

                auto ebpf = monitor_info->mutable_ebpf_info();
                ebpf->set_established_count(cur[TCP_ESTABLISHED] - last_state_counts_[TCP_ESTABLISHED]);
                ebpf->set_time_wait_count(cur[TCP_TIME_WAIT] - last_state_counts_[TCP_TIME_WAIT]);
                ebpf->set_close_wait_count(cur[TCP_CLOSE_WAIT] - last_state_counts_[TCP_CLOSE_WAIT]);
                ebpf->set_listening_count(cur[TCP_LISTEN] - last_state_counts_[TCP_LISTEN]);
                ebpf->set_syn_sent_count(cur[TCP_SYN_SENT] - last_state_counts_[TCP_SYN_SENT]);
                ebpf->set_fin_wait_count(
                    (cur[TCP_FIN_WAIT1] - last_state_counts_[TCP_FIN_WAIT1]) +
                    (cur[TCP_FIN_WAIT2] - last_state_counts_[TCP_FIN_WAIT2]));

                int64_t active = static_cast<int64_t>(cur[TCP_ESTABLISHED]) -
                                 static_cast<int64_t>(cur[TCP_CLOSE]);
                if (active < 0) active = 0;
                ebpf->set_total_active(active);
                ebpf->set_new_connection_rate(rate);

                std::sort(port_entries.begin(), port_entries.end(),
                    [](const auto& a, const auto& b) { return a.second > b.second; });
                int cnt = 0;
                for (auto& entry : port_entries)
                {
                    if (cnt++ >= 10) break;
                    auto ps = ebpf->add_port_stats();
                    ps->set_port(entry.first);
                    ps->set_count(entry.second);
                }

                last_events_ = ev;
                last_state_counts_ = cur;
                last_time_ = now;
            }
            catch (const std::exception& e)
            {
                std::cerr << "[BpfTcpMonitor] Exception: " << e.what() << std::endl;
            }
        }

    private:
        ebpf::BPF bpf_;
        bool bpf_ready_;
        uint64_t last_events_;
        std::vector<uint64_t> last_state_counts_;
        std::chrono::steady_clock::time_point last_time_;
    };

    BpfTcpMonitor::BpfTcpMonitor()
        : impl_(std::make_unique<Impl>())
    {}

    BpfTcpMonitor::~BpfTcpMonitor() = default;

    void BpfTcpMonitor::UpdateOnce(monitor::proto::MonitorInfo* monitor_info)
    {
        impl_->UpdateOnce(monitor_info);
    }

    void BpfTcpMonitor::Stop() {}
}  // namespace monitor

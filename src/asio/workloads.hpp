#pragma once

#include <latch>
#include <sisl/metrics/metrics.hpp>
#include "rpc_client.hpp"

namespace sisl {

struct WorkloadParams {
    uint64_t io_size;
    uint64_t io_count;
};

class WorkloadMetrics : public sisl::MetricsGroupWrapper {
public:
    explicit WorkloadMetrics(std::string const& name) : sisl::MetricsGroupWrapper{"Workload", name} {
        REGISTER_COUNTER(write_count, "Write count");
        // REGISTER_HISTOGRAM(drive_write_latency, "BlkStore drive write latency in us");

        register_me_to_farm();
    }

    WorkloadMetrics(const WorkloadMetrics&) = delete;
    WorkloadMetrics(WorkloadMetrics&&) noexcept = delete;
    WorkloadMetrics& operator=(const WorkloadMetrics&) = delete;
    WorkloadMetrics& operator=(WorkloadMetrics&&) noexcept = delete;

    ~WorkloadMetrics() { deregister_me_from_farm(); }
};

class Workload {
public:
    Workload(boost::asio::io_context& io_context, tcp::resolver::results_type const& endpoints,
             WorkloadParams const& params, std::string const& name);

    void run();

private:
    asio_client client_;
    WorkloadParams params_;
    WorkloadMetrics metrics_;
    std::latch io_write_latch_;
};

inline uint64_t get_elapsed_time_ns(const Clock::time_point& t1, const Clock::time_point& t2) {
    const std::chrono::nanoseconds ns{std::chrono::duration_cast< std::chrono::nanoseconds >(t2 - t1)};
    return ns.count();
}

inline uint64_t get_elapsed_time_us(const Clock::time_point& t1, const Clock::time_point& t2) {
    return get_elapsed_time_ns(t1, t2) / static_cast< uint64_t >(1000);
}

} // namespace sisl
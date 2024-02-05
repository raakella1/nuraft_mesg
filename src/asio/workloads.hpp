#pragma once

#include <latch>
#include <random>
#include <sisl/metrics/metrics.hpp>
#include "rpc_client.hpp"

namespace sisl {

struct WorkloadParams {
    std::string name;
    uint64_t io_size;
    uint64_t io_count;
    bool verify;
    std::string test_data_dir;
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

class rand_message {
public:
    rand_message(std::string const& name) : name_(name), msg_(fmt::format("{}@{}", name, get_rand_num())) {}
    rand_message(sisl::io_blob_safe&& buf) : msg_(std::string(buf.bytes(), buf.bytes() + buf.size())) {
        auto pos = msg_.find('@');
        name_ = msg_.substr(0, pos);
    }

    uint64_t get_rand_num() {
        static std::random_device dev;
        static std::mt19937 rng(dev());
        static std::uniform_int_distribution< std::mt19937::result_type > dist(1,
                                                                               std::numeric_limits< uint64_t >::max());
        return dist(rng);
    }

    std::string name_;
    std::string msg_;
};

class Workload {
public:
    Workload(boost::asio::io_context& io_context, tcp::resolver::results_type const& endpoints,
             WorkloadParams const& params);

    void run();

private:
    void write();
    void write_verify();
    std::vector< std::byte > generate_data(uint64_t const size);
    void verify();

private:
    asio_client client_;
    WorkloadParams params_;
    WorkloadMetrics metrics_;
    std::atomic_uint32_t io_write_count_{0};
    std::promise< void > io_write_promise_;
    std::vector< rand_message > data_;
};

inline uint64_t get_elapsed_time_ns(const Clock::time_point& t1, const Clock::time_point& t2) {
    const std::chrono::nanoseconds ns{std::chrono::duration_cast< std::chrono::nanoseconds >(t2 - t1)};
    return ns.count();
}

inline uint64_t get_elapsed_time_us(const Clock::time_point& t1, const Clock::time_point& t2) {
    return get_elapsed_time_ns(t1, t2) / static_cast< uint64_t >(1000);
}

} // namespace sisl
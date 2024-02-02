#include <random>
#include "workloads.hpp"

namespace sisl {

Workload::Workload(boost::asio::io_context& io_context, tcp::resolver::results_type const& endpoints,
                   WorkloadParams const& params, std::string const& name) :
        client_(io_context, endpoints,
                [this](auto ec) {
                    static std::atomic_int count = 0;
                    LOGINFO("io write completion, count: {}", count++)
                    RELEASE_ASSERT(!ec, "io error: {}", ec.message());
                    io_write_latch_.count_down();
                }),
        params_(params),
        metrics_(name),
        io_write_latch_(params.io_count) {}

void Workload::run() {
    // random data of size io_size_
    auto data = std::vector< std::byte >(params_.io_size);
    // fill the data with random data
    static std::independent_bits_engine< std::default_random_engine, CHAR_BIT, unsigned char > rbe;
    for (auto& b : data) {
        b = std::byte{rbe()};
    }
    for (uint64_t i = 0; i < params_.io_count; ++i) {
        client_.write(data);
        COUNTER_INCREMENT(metrics_, write_count, 1);
    }
    io_write_latch_.wait();
}
} // namespace sisl

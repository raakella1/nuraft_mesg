#include <filesystem>
#include <chrono>
#include "workloads.hpp"

namespace sisl {

Workload::Workload(boost::asio::io_context& io_context, tcp::resolver::results_type const& endpoints,
                   WorkloadParams const& params) :
        client_(io_context, endpoints,
                [this](auto ec) {
                    RELEASE_ASSERT(!ec, "io error: {}", ec.message());
                    if (++io_write_count_ < params_.io_count) {
                        params_.verify ? write_verify() : write();
                    } else {
                        io_write_promise_.set_value();
                    }
                }),
        params_(params),
        metrics_(params.name) {}

void Workload::run() {
    params_.verify ? write_verify() : write();
    io_write_promise_.get_future().wait();
    verify();
}

void Workload::write() {
    client_.write(generate_data(params_.io_size));
    COUNTER_INCREMENT(metrics_, write_count, 1);
}

void Workload::write_verify() {
    data_.emplace_back(rand_message(params_.name));
    client_.write(data_.back().msg_);
}

std::vector< std::byte > Workload::generate_data(uint64_t const size) {
    auto data = std::vector< std::byte >(size);
    // fill the data with random data
    static std::independent_bits_engine< std::default_random_engine, CHAR_BIT, unsigned char > rbe;
    for (auto& b : data) {
        b = std::byte{rbe()};
    }
    return data;
}

void Workload::verify() {
    if (!params_.verify) { return; }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto file_name = fmt::format("{}/{}", params_.test_data_dir, params_.name);

    RELEASE_ASSERT(std::filesystem::exists(file_name), "File {} not found", file_name);
    std::ifstream infile(file_name);
    std::string line;
    size_t line_no = 0;
    while (std::getline(infile, line)) {
        if (line.empty()) { break; }
        RELEASE_ASSERT(line == data_[line_no].msg_, "File {} does not match. Expected = {}, got {}", file_name,
                       data_[line_no].msg_, line);
        line_no++;
    }
    RELEASE_ASSERT(line_no == data_.size(), "File {} does not match. Expected = {}, got {}", file_name, data_.size(),
                   line_no);
}

} // namespace sisl

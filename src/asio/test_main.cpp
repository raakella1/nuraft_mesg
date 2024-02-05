#include <sisl/options/options.h>
#include <sisl/logging/logging.h>
#include "workloads.hpp"

SISL_OPTION_GROUP(asio_test,
                  (svr_ip_addr, "", "svr_ip_addr", "svr_ip_addr",
                   cxxopts::value< std::string >()->default_value("127.0.0.1"), ""),
                  (svr_port, "", "svr_port", "svr_port", cxxopts::value< std::string >()->default_value("1991"), ""),
                  (io_size, "", "io_size", "io_size", cxxopts::value< uint32_t >()->default_value("1000"),
                   "io size in bytes"),
                  (io_count, "", "io_count", "io_count", cxxopts::value< uint32_t >()->default_value("10000"), ""),
                  (num_clients, "", "num_clients", "num_clients", cxxopts::value< uint32_t >()->default_value("1"), ""),
                  (io_threads, "", "io_threads", "io_threads", cxxopts::value< uint32_t >()->default_value("2"), ""),
                  (verify, "", "verify", "verify", cxxopts::value< bool >()->default_value("false"), ""),
                  (test_data_dir, "", "test_data_dir", "test_data_dir",
                   cxxopts::value< std::string >()->default_value("test_data"), "test data directory"))

#define TEST_OPTIONS logging, asio_test
SISL_OPTIONS_ENABLE(TEST_OPTIONS)
SISL_LOGGING_INIT()

using boost::asio::ip::tcp;

int main(int argc, char** argv) {
    SISL_OPTIONS_LOAD(argc, argv, TEST_OPTIONS)
    sisl::logging::SetLogger("asio_test");
    sisl::logging::install_crash_handler();
    spdlog::set_pattern("[%D %T.%e] [%n] [%^%l%$] [%t] %v");

    boost::asio::io_context io_context;
    tcp::resolver resolver(io_context);
    auto endpoints =
        resolver.resolve(SISL_OPTIONS["svr_ip_addr"].as< std::string >(), SISL_OPTIONS["svr_port"].as< std::string >());

    auto work = boost::asio::make_work_guard(io_context);
    std::vector< std::thread > io_threads;
    for (uint16_t i = 0; i < SISL_OPTIONS["io_threads"].as< uint32_t >(); ++i) {
        io_threads.emplace_back([&io_context] { io_context.run(); });
    }

    LOGINFO("Starting workloads");
    std::vector< std::thread > workload_threads;

    for (uint32_t i = 0; i < SISL_OPTIONS["num_clients"].as< uint32_t >(); ++i) {
        workload_threads.emplace_back([&io_context, endpoints, i] {
            auto w = sisl::Workload(
                io_context, endpoints,
                sisl::WorkloadParams{fmt::format("workload_{}", i), SISL_OPTIONS["io_size"].as< uint32_t >(),
                                     SISL_OPTIONS["io_count"].as< uint32_t >(), SISL_OPTIONS["verify"].as< bool >(),
                                     SISL_OPTIONS["test_data_dir"].as< std::string >()});
            w.run();
        });
    }

    LOGINFO("Waiting for workloads to complete");
    for (auto& t : workload_threads) {
        t.join();
    }
    io_context.stop();
    for (auto& t : io_threads) {
        t.join();
    }
    LOGINFO("Workloads completed");
    return 0;
}
#include <filesystem>
#include <sisl/options/options.h>
#include <sisl/logging/logging.h>
#include "rpc_server.hpp"
#include "workloads.hpp"

SISL_OPTION_GROUP(asio_server,
                  (svr_port, "", "svr_port", "svr_port", cxxopts::value< uint32_t >()->default_value("1991"), ""),
                  (num_threads, "", "num_threads", "num_threads", cxxopts::value< uint32_t >()->default_value("2"), ""),
                  (verify, "", "verify", "verify", cxxopts::value< bool >()->default_value("false"), ""),
                  (test_data_dir, "", "test_data_dir", "test_data_dir",
                   cxxopts::value< std::string >()->default_value("test_data"), "test data directory"))

#define SVR_OPTIONS logging, asio_server
SISL_OPTIONS_ENABLE(SVR_OPTIONS)
SISL_LOGGING_INIT()

using boost::asio::ip::tcp;

int main(int argc, char** argv) {
    SISL_OPTIONS_LOAD(argc, argv, SVR_OPTIONS)
    sisl::logging::SetLogger("asio_test");
    sisl::logging::install_crash_handler();
    spdlog::set_pattern("[%D %T.%e] [%n] [%^%l%$] [%t] %v");

    sisl::asio_server svr(tcp::endpoint(tcp::v4(), SISL_OPTIONS["svr_port"].as< uint32_t >()),
                          SISL_OPTIONS["num_threads"].as< uint32_t >(), [](auto const ec, auto&& buf) {
                              RELEASE_ASSERT(!ec, "io error: {}", ec.message());
                              if (!SISL_OPTIONS["verify"].as< bool >()) { return; }
                              sisl::rand_message msg(std::move(buf));
                              std::ofstream outfile(
                                  fmt::format("{}/{}", SISL_OPTIONS["test_data_dir"].as< std::string >(), msg.name_),
                                  std::ios_base::app);
                              outfile << msg.msg_ << "\n";
                          });
    svr.run();
    return 0;
}
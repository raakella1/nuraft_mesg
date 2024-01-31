#include <sisl/options/options.h>
#include <sisl/logging/logging.h>
#include "rpc_server.hpp"

SISL_OPTION_GROUP(asio_server,
                  (svr_port, "", "svr_port", "svr_port", cxxopts::value< uint32_t >()->default_value("1991"), ""))

#define SVR_OPTIONS logging, asio_server
SISL_OPTIONS_ENABLE(SVR_OPTIONS)
SISL_LOGGING_INIT()

using boost::asio::ip::tcp;

int main(int argc, char** argv) {
    SISL_OPTIONS_LOAD(argc, argv, SVR_OPTIONS)
    sisl::logging::SetLogger("asio_test");
    sisl::logging::install_crash_handler();
    spdlog::set_pattern("[%D %T.%e] [%n] [%^%l%$] [%t] %v");
    sisl::asio_server svr(tcp::endpoint(tcp::v4(), SISL_OPTIONS["svr_port"].as< uint32_t >()), 2);
    svr.run();
    return 0;
}
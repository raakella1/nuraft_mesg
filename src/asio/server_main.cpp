#include "rpc_server.hpp"

using boost::asio::ip::tcp;

int main() {
    sisl::asio_server svr(tcp::endpoint(tcp::v4(), 1991), 2);
    svr.run();
    return 0;
}
#pragma once

#include "session.hpp"

namespace sisl {
class asio_client {
public:
    asio_client(boost::asio::io_context& io_context, tcp::resolver::results_type const& endpoints);

    void write(std::string const& message);

private:
    void do_connect(tcp::resolver::results_type const& endpoints);

private:
    tcp::socket socket_;
    std::shared_ptr< Session > session_;
};
} // namespace sisl
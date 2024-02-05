#pragma once

#include "session.hpp"

namespace sisl {
class asio_server {
public:
    asio_server(tcp::endpoint const& endpoint, uint16_t num_threads, read_completion_cb const& cb = nullptr);

    void run();
    void stop();

private:
    void do_accept();

private:
    boost::asio::io_context io_context_;
    tcp::acceptor acceptor_;
    uint16_t num_threads_;
    std::vector< std::thread > threads_;
    read_completion_cb cb_;
};
} // namespace sisl
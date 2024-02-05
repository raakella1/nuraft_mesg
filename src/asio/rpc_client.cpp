#include <iostream>
#include <thread>
#include <string>

#include "rpc_client.hpp"

namespace sisl {
asio_client::asio_client(boost::asio::io_context& io_context, tcp::resolver::results_type const& endpoints,
                         write_completion_cb const& cb) :
        socket_(io_context), cb_(cb) {
    do_connect(endpoints);
}

void asio_client::do_connect(tcp::resolver::results_type const& endpoints) {
    boost::asio::connect(socket_, endpoints);
    session_ = std::make_shared< Session >(std::move(socket_), cb_, nullptr);
    session_->start();
}

void asio_client::do_connect_async(tcp::resolver::results_type const& endpoints) {
    boost::asio::async_connect(socket_, endpoints, [this](boost::system::error_code ec, tcp::endpoint) {
        if (!ec) {
            std::cout << "Connected" << std::endl;
            session_ = std::make_shared< Session >(std::move(socket_));
            session_->start();
        } else {
            std::cerr << "Connect error: " << ec.message() << std::endl;
        }
    });
}

} // namespace sisl
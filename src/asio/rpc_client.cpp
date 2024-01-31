#include <iostream>
#include <thread>
#include <string>

#include "rpc_client.hpp"

namespace sisl {
asio_client::asio_client(boost::asio::io_context& io_context, tcp::resolver::results_type const& endpoints) :
        socket_(io_context) {
    do_connect(endpoints);
}

void asio_client::write(std::string const& message) {
    std::cout << "Sending message: " << message << std::endl;
    sisl::io_blob_safe buf(message.size(), 512);
    std::memcpy(buf.bytes(), message.data(), message.size());
    session_->write(std::move(buf));
}

void asio_client::do_connect(tcp::resolver::results_type const& endpoints) {
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
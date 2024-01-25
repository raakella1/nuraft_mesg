#include <iostream>
#include <thread>
#include <string>

#include "session.hpp"

class asio_client {
public:
    asio_client(boost::asio::io_context& io_context, tcp::resolver::results_type const& endpoints) :
            socket_(io_context) {
        do_connect(endpoints);
    }

    void write(std::string const& message) {
        std::cout << "Sending message: " << message << std::endl;
        sisl::io_blob buf(message.size());
        std::memcpy(buf.bytes(), message.data(), message.size());
        session_->write(buf);
    }

private:
    void do_connect(tcp::resolver::results_type const& endpoints) {
        boost::asio::async_connect(socket_, endpoints, [this](boost::system::error_code ec, tcp::endpoint) {
            if (!ec) {
                std::cout << "Connected" << std::endl;
                session_ = std::make_shared< session >(std::move(socket_));
                session_->start();
            } else {
                std::cerr << "Connect error: " << ec.message() << std::endl;
            }
        });
    }

private:
    tcp::socket socket_;
    std::shared_ptr< session > session_;
};

int main() {
    boost::asio::io_context io_context;
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("127.0.0.1", "1991");
    asio_client client(io_context, endpoints);
    std::thread t([&io_context]() { io_context.run(); });

    std::string message;
    std::cout << "Enter message: ";
    while (std::getline(std::cin, message)) {
        if (message == "exit") { break; }
        client.write(message);
    }
    std::cout << "Exiting..." << std::endl;
    t.join();
    return 0;
}
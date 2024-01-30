#include <iostream>
#include <thread>
#include <string>

#include "session.hpp"

class asio_server {
public:
    asio_server(tcp::endpoint const& endpoint, uint16_t num_threads) :
            acceptor_(io_context_, endpoint), num_threads_(num_threads) {
        do_accept();
    }

    void run() {
        for (std::size_t i = 0; i < num_threads_; ++i) {
            threads_.emplace_back([this] { io_context_.run(); });
        }

        for (auto& t : threads_) {
            t.join();
        }
    }

    void stop() { io_context_.stop(); }

private:
    void do_accept() {
        acceptor_.async_accept(boost::asio::make_strand(io_context_),
                               [this](boost::system::error_code ec, tcp::socket socket) {
                                   std::cout << "Accepted connection" << std::endl;
                                   if (!ec) {
                                       std::make_shared< session >(std::move(socket))->start();
                                   } else {
                                       std::cerr << "Accept error: " << ec.message() << std::endl;
                                   }
                                   do_accept();
                               });
    }

    boost::asio::io_context io_context_;
    tcp::acceptor acceptor_;
    uint16_t num_threads_;
    std::vector< std::thread > threads_;
};

int main() {

    asio_server svr(tcp::endpoint(tcp::v4(), 1991), 2);
    svr.run();
    return 0;
}
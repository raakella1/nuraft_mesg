#include <iostream>
#include <thread>
#include <string>

#include "session.hpp"

class asio_server {
public:
    asio_server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint) : acceptor_(io_context, endpoint) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            std::cout << "Accepted connection" << std::endl;
            if (!ec) {
                std::make_shared< session >(std::move(socket))->start();
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }

            do_accept();
        });
    }

    tcp::acceptor acceptor_;
};

int main() {
    boost::asio::io_context ctx;
    asio_server svr(ctx, tcp::endpoint(tcp::v4(), 1991));

    // Run the IO context
    ctx.run();

    return 0;
}
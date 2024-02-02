#include <sisl/logging/logging.h>
#include "rpc_server.hpp"

namespace sisl {

asio_server::asio_server(tcp::endpoint const& endpoint, uint16_t num_threads) :
        acceptor_(io_context_, endpoint), num_threads_(num_threads) {
    do_accept();
}

void asio_server::run() {
    for (std::size_t i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this] { io_context_.run(); });
    }

    for (auto& t : threads_) {
        t.join();
    }
}

void asio_server::stop() { io_context_.stop(); }

void asio_server::do_accept() {
    acceptor_.async_accept(boost::asio::make_strand(io_context_),
                           [this](boost::system::error_code ec, tcp::socket socket) {
                               LOGINFO("accepting connection")
                               if (!ec) {
                                   std::make_shared< Session >(std::move(socket))->start();
                               } else {
                                   LOGERROR("accept error: {}", ec.message());
                               }
                               do_accept();
                           });
}

} // namespace sisl
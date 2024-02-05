#pragma once

#include "session.hpp"

namespace sisl {

class asio_client {
public:
    asio_client(boost::asio::io_context& io_context, tcp::resolver::results_type const& endpoints,
                write_completion_cb const& cb);

    template < typename T >
    void write(T const& message) {
        sisl::io_blob_safe buf(message.size(), 512);
        std::memcpy(buf.bytes(), message.data(), message.size());
        session_->write(std::move(buf));
    }

private:
    void do_connect(tcp::resolver::results_type const& endpoints);
    void do_connect_async(tcp::resolver::results_type const& endpoints);

private:
    tcp::socket socket_;
    std::shared_ptr< Session > session_;
    write_completion_cb cb_;
};
} // namespace sisl
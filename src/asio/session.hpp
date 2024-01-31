#pragma once

#include <boost/asio.hpp>
#include "message.hpp"

namespace sisl {
using boost::asio::ip::tcp;
using async_handler_t = std::function< void(boost::system::error_code, size_t) >;

class Session : public std::enable_shared_from_this< Session > {
public:
    Session(tcp::socket socket);
    ~Session();

    void start();

    void write(sisl::io_blob_safe&& buf);

private:
    void do_read_header();
    void do_read_body();

    void do_write_header();
    void do_write_body();

    void pass_through() {}

private:
    tcp::socket socket_;
    HeaderMessage receive_header_;
    Message receive_body_;
    HeaderMessage send_header_;
    Message send_body_;
};

} // namespace sisl
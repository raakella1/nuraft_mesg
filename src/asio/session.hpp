#pragma once

#include <boost/asio.hpp>
#include "message.hpp"

namespace sisl {
using boost::asio::ip::tcp;
using write_completion_cb = std::function< void(boost::system::error_code const) >;
using read_completion_cb = std::function< void(boost::system::error_code const, sisl::io_blob_safe&&) >;

class Session : public std::enable_shared_from_this< Session > {
public:
    Session(tcp::socket socket);
    Session(tcp::socket socket, write_completion_cb const& write_cb, read_completion_cb const& read_cb);
    ~Session();

    void start();

    void write(sisl::io_blob_safe&& buf);

private:
    void do_read_header();
    void do_read_body();

    void do_write_header();
    void do_write_body();

private:
    tcp::socket socket_;
    HeaderMessage receive_header_;
    Message receive_body_;
    HeaderMessage send_header_;
    Message send_body_;
    write_completion_cb write_cb_;
    read_completion_cb read_cb_;
};

} // namespace sisl
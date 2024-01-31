#pragma once

#include <boost/asio.hpp>
#include "message.hpp"

namespace sisl {
using boost::asio::ip::tcp;
using async_handler_t = std::function< void(boost::system::error_code, size_t) >;
using completion_cb = std::function< void(boost::system::error_code) >;

class Session : public std::enable_shared_from_this< Session > {
public:
    Session(tcp::socket socket);
    Session(tcp::socket socket, completion_cb const& write_cb, completion_cb const& read_cb);
    ~Session();

    void start();

    void write(sisl::io_blob_safe&& buf);

private:
    void do_read_header(boost::system::error_code);
    void do_read_body(boost::system::error_code);
    void do_read_completion(boost::system::error_code ec);

    void do_write_header(boost::system::error_code);
    void do_write_body(boost::system::error_code);
    void do_write_completion(boost::system::error_code ec);

    void pass_through() {}

private:
    tcp::socket socket_;
    HeaderMessage receive_header_;
    Message receive_body_;
    HeaderMessage send_header_;
    Message send_body_;
    completion_cb write_cb_;
    completion_cb read_cb_;
};

} // namespace sisl
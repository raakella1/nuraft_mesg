#pragma once

#include <boost/asio.hpp>
#include "message.hpp"

using boost::asio::ip::tcp;
using async_handler_t = std::function< void(boost::system::error_code, size_t) >;

#define ASYNC_MSG_HELPER(direction, msg, partial_handler, finished_handler)                                            \
    auto self(shared_from_this());                                                                                     \
    socket_.async_##direction(boost::asio::buffer(msg.pos(), msg.remaining_size()), MSG_ZEROCOPY,                      \
                              [this, self](boost::system::error_code ec, size_t length) {                              \
                                  if (!ec) {                                                                           \
                                      msg.move_offset(length);                                                         \
                                      if (msg.finished()) {                                                            \
                                          msg.reset();                                                                 \
                                          finished_handler();                                                          \
                                      } else {                                                                         \
                                          partial_handler();                                                           \
                                      }                                                                                \
                                  } else {                                                                             \
                                      std::cerr << "Read error: " << ec.message() << std::endl;                        \
                                  }                                                                                    \
                              });

class session : public std::enable_shared_from_this< session > {
public:
    session(tcp::socket socket) : socket_(std::move(socket)) {}
    ~session() { socket_.close(); }

    void start() { do_read_header(); }

    void write(sisl::io_blob_safe&& buf) {
        send_header_.serialize(buf.size());
        send_body_.set_buffer(std::move(buf));
        do_write_header();
    }

private:
    void do_read_header() {
        // std::cout << std::string(receive_body_.bytes(), receive_body_.bytes() + receive_body_.size()) << std::endl;
        ASYNC_MSG_HELPER(receive, receive_header_, do_read_header, do_read_body);
    }
    void do_read_body() {
        receive_body_.set_buffer(sisl::io_blob_safe(receive_header_.deserialize(), 512));
        ASYNC_MSG_HELPER(receive, receive_body_, do_read_body, do_read_header);
    }

    void do_write_header() { ASYNC_MSG_HELPER(send, send_header_, do_write_header, do_write_body); }
    void do_write_body() { ASYNC_MSG_HELPER(send, send_body_, do_write_body, pass_through); }

    void pass_through() {}

private:
    tcp::socket socket_;
    HeaderMessage receive_header_;
    Message receive_body_;
    HeaderMessage send_header_;
    Message send_body_;
};
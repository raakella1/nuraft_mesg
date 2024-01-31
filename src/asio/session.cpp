#include "session.hpp"

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

namespace sisl {

Session::Session(tcp::socket socket) : socket_(std::move(socket)) {}
Session::~Session() { socket_.close(); }

void Session::start() { do_read_header(); }

void Session::write(sisl::io_blob_safe&& buf) {
    send_header_.serialize(buf.size());
    send_body_.set_buffer(std::move(buf));
    do_write_header();
}

void Session::do_read_header() { ASYNC_MSG_HELPER(receive, receive_header_, do_read_header, do_read_body); }
void Session::do_read_body() {
    receive_body_.set_buffer(sisl::io_blob_safe(receive_header_.deserialize(), 512));
    ASYNC_MSG_HELPER(receive, receive_body_, do_read_body, do_read_header);
}

void Session::do_write_header() { ASYNC_MSG_HELPER(send, send_header_, do_write_header, do_write_body); }
void Session::do_write_body() { ASYNC_MSG_HELPER(send, send_body_, do_write_body, pass_through); }

} // namespace sisl
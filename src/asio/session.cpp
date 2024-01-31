#include <sisl/logging/logging.h>
#include "session.hpp"

#define ASYNC_MSG_HELPER(direction, msg, partial_handler, finished_handler)                                            \
    auto self(shared_from_this());                                                                                     \
    socket_.async_##direction(boost::asio::buffer(msg.pos(), msg.remaining_size()), MSG_ZEROCOPY,                      \
                              [this, self](boost::system::error_code ec, size_t length) {                              \
                                  if (!ec) {                                                                           \
                                      msg.move_offset(length);                                                         \
                                      if (msg.finished()) {                                                            \
                                          msg.reset();                                                                 \
                                          finished_handler(ec);                                                        \
                                      } else {                                                                         \
                                          partial_handler(ec);                                                         \
                                      }                                                                                \
                                  } else {                                                                             \
                                      socket_.close();                                                                 \
                                  }                                                                                    \
                              });

namespace sisl {

Session::Session(tcp::socket socket) : socket_(std::move(socket)) {}
Session::Session(tcp::socket socket, completion_cb const& write_cb, completion_cb const& read_cb) :
        Session(std::move(socket)) {
    write_cb_ = write_cb;
    read_cb_ = read_cb;
}
Session::~Session() { socket_.close(); }

void Session::start() { do_read_header(boost::system::error_code()); }

void Session::write(sisl::io_blob_safe&& buf) {
    send_header_.serialize(buf.size());
    send_body_.set_buffer(std::move(buf));
    do_write_header(boost::system::error_code());
}

void Session::do_read_header(boost::system::error_code) {
    ASYNC_MSG_HELPER(receive, receive_header_, do_read_header, do_read_body);
}
void Session::do_read_body(boost::system::error_code) {
    receive_body_.set_buffer(sisl::io_blob_safe(receive_header_.deserialize(), 512));
    ASYNC_MSG_HELPER(receive, receive_body_, do_read_body, do_read_completion);
}
void Session::do_read_completion(boost::system::error_code ec) {
    if (read_cb_) { read_cb_(ec); }
    do_read_header(boost::system::error_code());
}

void Session::do_write_header(boost::system::error_code) {
    ASYNC_MSG_HELPER(send, send_header_, do_write_header, do_write_body);
}
void Session::do_write_body(boost::system::error_code) {
    ASYNC_MSG_HELPER(send, send_body_, do_write_body, do_write_completion);
}
void Session::do_write_completion(boost::system::error_code ec) {
    if (write_cb_) { write_cb_(ec); }
}

} // namespace sisl
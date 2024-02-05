#include <sisl/logging/logging.h>
#include "session.hpp"

namespace sisl {

Session::Session(tcp::socket socket) : socket_(std::move(socket)) {}
Session::Session(tcp::socket socket, write_completion_cb const& write_cb, read_completion_cb const& read_cb) :
        Session(std::move(socket)) {
    write_cb_ = write_cb;
    read_cb_ = read_cb;
}
Session::~Session() { socket_.close(); }

void Session::start() { do_read_header(); }

void Session::write(sisl::io_blob_safe&& buf) {
    send_header_.serialize(buf.size());
    send_body_.set_buffer(std::move(buf));
    do_write_header();
}

void Session::do_read_header() {
    auto self(shared_from_this());
    socket_.async_receive(boost::asio::buffer(receive_header_.pos(), receive_header_.remaining_size()), MSG_ZEROCOPY,
                          [this, self](boost::system::error_code ec, size_t length) {
                              if (!ec) {
                                  receive_header_.move_offset(length);
                                  if (receive_header_.finished()) {
                                      receive_header_.reset_offset();
                                      do_read_body();
                                  } else {
                                      do_read_header();
                                  }
                              } else {
                                  LOGTRACE("do_read_header error: {}", ec.message());
                                  socket_.close();
                              }
                          });
}

void Session::do_read_body() {
    receive_body_.set_buffer(sisl::io_blob_safe(receive_header_.deserialize(), 512));
    auto self(shared_from_this());
    socket_.async_receive(boost::asio::buffer(receive_body_.pos(), receive_body_.remaining_size()), MSG_ZEROCOPY,
                          [this, self](boost::system::error_code ec, size_t length) {
                              if (!ec) {
                                  receive_body_.move_offset(length);
                                  if (receive_body_.finished()) {
                                      receive_body_.reset_offset();
                                      if (read_cb_) { read_cb_(ec, receive_body_.move_buffer()); }
                                      do_read_header();
                                  } else {
                                      do_read_body();
                                  }
                              } else {
                                  socket_.close();
                              }
                          });
}

void Session::do_write_header() {
    auto self(shared_from_this());
    socket_.async_send(boost::asio::buffer(send_header_.pos(), send_header_.remaining_size()), MSG_ZEROCOPY,
                       [this, self](boost::system::error_code ec, size_t length) {
                           if (!ec) {
                               send_header_.move_offset(length);
                               if (send_header_.finished()) {
                                   send_header_.reset_offset();
                                   do_write_body();
                               } else {
                                   do_write_header();
                               }
                           } else {
                               LOGTRACE("do_write_header error: {}", ec.message());
                               if (write_cb_) { write_cb_(ec); }
                               socket_.close();
                           }
                       });
}

void Session::do_write_body() {
    auto self(shared_from_this());
    socket_.async_send(boost::asio::buffer(send_body_.pos(), send_body_.remaining_size()), MSG_ZEROCOPY,
                       [this, self](boost::system::error_code ec, size_t length) {
                           if (!ec) {
                               send_body_.move_offset(length);
                               if (send_body_.finished()) {
                                   send_body_.reset_offset();
                                   if (write_cb_) { write_cb_(ec); }
                               } else {
                                   do_write_body();
                               }
                           } else {
                               LOGTRACE("do_write_body error: {}", ec.message());
                               if (write_cb_) { write_cb_(ec); }
                               socket_.close();
                           }
                       });
}

} // namespace sisl
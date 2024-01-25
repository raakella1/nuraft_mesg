#pragma once

#include <boost/asio.hpp>
#include <sisl/fds/buffer.hpp>

using boost::asio::ip::tcp;
using async_handler_t = std::function< void(boost::system::error_code, size_t) >;
uint32_t const read_sz = 6;

class session : public std::enable_shared_from_this< session > {
public:
    session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        receive_buffer_.buf_alloc(read_sz);
        do_read();
    }

    void write(sisl::io_blob const& buf) {
        send_buffer_ = buf;
        do_write();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        std::cout << "Waiting for message" << std::endl;
        std::cout << "zero_cpy: " << MSG_ZEROCOPY << std::endl;
        socket_.async_receive(
            boost::asio::buffer(receive_buffer_.bytes() + bytes_received_, receive_buffer_.size() - bytes_received_),
            MSG_ZEROCOPY, [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    std::cout << "lenght: " << length << std::endl;
                    bytes_received_ += length;
                    if (bytes_received_ == receive_buffer_.size()) {
                        std::cout << "Received message: "
                                  << std::string(receive_buffer_.bytes(),
                                                 receive_buffer_.bytes() + receive_buffer_.size())
                                  << std::endl;
                        bytes_received_ = 0;
                    } else {
                        std::cout << "Received partial bytes: " << length << std::endl;
                    }
                    do_read();
                } else {
                    std::cerr << "Read error: " << ec.message() << std::endl;
                }
            });
    }

    void do_write() {
        auto self(shared_from_this());
        socket_.async_send(boost::asio::buffer(send_buffer_.bytes() + bytes_sent_, send_buffer_.size() - bytes_sent_),
                           MSG_ZEROCOPY, [this, self](boost::system::error_code ec, size_t length) {
                               if (!ec) {
                                   bytes_sent_ += length;
                                   if (bytes_sent_ == send_buffer_.size()) {
                                       std::cout << "Sent message" << std::endl;
                                       bytes_sent_ = 0;
                                   } else {
                                       std::cout << "Sent partial bytes: " << length << std::endl;
                                       do_write();
                                   }
                               } else {
                                   std::cerr << "Send error: " << ec.message() << std::endl;
                               }
                           });
    }

    tcp::socket socket_;
    sisl::io_blob receive_buffer_;
    sisl::io_blob send_buffer_;
    size_t bytes_received_{0};
    size_t bytes_sent_{0};
};
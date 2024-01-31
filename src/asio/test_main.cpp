#include "rpc_client.hpp"

using boost::asio::ip::tcp;

int main() {
    boost::asio::io_context io_context;
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("127.0.0.1", "1991");
    sisl::asio_client client(io_context, endpoints);

    std::vector< std::thread > threads_;
    uint16_t num_threads_ = 2;
    for (uint16_t i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([&io_context] { io_context.run(); });
    }

    std::string message;
    std::cout << "Enter message: ";
    while (std::getline(std::cin, message)) {
        if (message == "exit") { break; }
        client.write(message);
    }
    std::cout << "Exiting..." << std::endl;
    io_context.stop();
    for (auto& t : threads_) {
        t.join();
    }
    return 0;
}
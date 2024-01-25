#pragma once

#include <boost/asio.hpp>
#include <sisl/fds/buffer.hpp>

namespace nu_asio {

class asio_buffer {
public:
    using sisl::blob::blob;

private:
    size_t bytes_read{0};
};
} // namespace nu_asio
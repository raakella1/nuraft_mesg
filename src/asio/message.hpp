#pragma once

#include <sisl/fds/buffer.hpp>

namespace sisl {
class Message {
public:
    Message() = default;
    virtual ~Message() = default;
    Message(uint32_t sz) : buffer_{sz, 512} {}

    void set_buffer(sisl::io_blob_safe&& buf) {
        buffer_ = std::move(buf);
        offset_ = 0;
    }
    void move_offset(size_t sz) { offset_ += sz; }
    uint8_t* pos() { return buffer_.bytes() + offset_; }
    uint8_t* bytes() { return buffer_.bytes(); }
    uint32_t remaining_size() const { return buffer_.size() - offset_; }
    uint32_t size() const { return buffer_.size(); }
    bool finished() const { return offset_ == buffer_.size(); }
    void reset() { offset_ = 0; }

private:
    size_t offset_{0};
    sisl::io_blob_safe buffer_;
};

class HeaderMessage : public Message {
public:
    static constexpr uint32_t header_size = sizeof(uint32_t);
    HeaderMessage() : Message(header_size) {}

    void serialize(uint32_t sz) { *reinterpret_cast< uint32_t* >(Message::bytes()) = sz; }
    uint32_t deserialize() { return *reinterpret_cast< uint32_t* >(Message::bytes()); }
};

} // namespace sisl
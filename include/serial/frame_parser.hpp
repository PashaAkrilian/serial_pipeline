#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace serial {

struct Frame {
    std::vector<std::uint8_t>payload;
};

class FrameParser {
public:
    static constexpr std::uint8_t kSync1 = 0xAA;
    static constexpr std::uint8_t kSync2 = 0x55;

    //menerima potongan byte, mengembalikan frame yang sudah lengkap dan valid
    std::vector<Frame> feed(const std::uint8_t* data, std::size_t len);

    std::vector<Frame> feed(const std::vector<std::uint8_t>& data) {
        return feed(data.data(), data.size());
    }

private:
    enum class State { Sync1, Sync2, Length, Payload, Checksum};

    State state_ = State::Sync1;
    std::uint8_t length_ = 0;
    std::uint8_t checksum_ = 0;
    std::vector<std::uint8_t> payload_;
};

} // namespace serial

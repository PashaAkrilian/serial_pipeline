#include "serial/frame_parser.hpp"

namespace serial {

std::vector<Frame> FrameParser::feed(const std::uint8_t* data, std::size_t len) {
    std::vector<Frame> frames;

    for (std::size_t i = 0; i < len; ++i) {
        const std::uint8_t b = data[i];

        switch (state_) {
            case State::Sync1:
                if (b == kSync1) state_ = State::Sync2;
                break;

            case State::Sync2:
                if (b == kSync2) {
                    state_ = State::Length;
                } else if (b != kSync1) {
                    state_ = State::Sync1;
                }
                break;

            case State::Length:
                length_ = b;
                checksum_ = b;
                payload_.clear();
                payload_.reserve(length_);
                state_ = (length_ == 0) ? State::Checksum : State::Payload;
                break;

            case State::Payload:
                payload_.push_back(b);
                checksum_ ^= b;
                if (payload_.size() == length_) {
                    state_ = State::Checksum;
                }
                break;

            case State::Checksum:
                if (b == checksum_) {
                    frames.push_back(Frame{payload_});
                }
                payload_.clear();
                state_ = State::Sync1;
                break;
        }
    }

    return frames;
}

}  // namespace serial
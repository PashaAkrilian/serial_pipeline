#include "gtest/gtest.h"
#include <cstdint>
#include <vector>
#include "serial/frame_parser.hpp"

using Bytes = std::vector<std::uint8_t>;

// Membuat frame lengkap dengan checksum yang benar
Bytes make_frame(const Bytes& payload) {
    const auto len = static_cast<std::uint8_t>(payload.size());
    Bytes f{0xAA, 0x55, len};
    std::uint8_t chk = len;
    for (std::uint8_t b : payload) {
        f.push_back(b);
        chk ^= b;
    }
    f.push_back(chk);
    return f;
}

TEST(FrameParserTest, SingleFrame) {
    serial::FrameParser p;
    auto frames = p.feed(make_frame({1, 2, 3}));
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].payload, (Bytes{1, 2, 3}));
}

TEST(FrameParserTest, FrameSplitAcrossCalls) {
    serial::FrameParser p;
    Bytes f = make_frame({10, 20, 30, 40});
    Bytes first(f.begin(), f.begin() + 4);
    Bytes second(f.begin() + 4, f.end());

    EXPECT_TRUE(p.feed(first).empty());
    auto frames = p.feed(second);
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].payload, (Bytes{10, 20, 30, 40}));
}

TEST(FrameParserTest, MultipleFramesInOneChunk) {
    serial::FrameParser p;
    Bytes data = make_frame({1});
    Bytes second = make_frame({2, 3});
    data.insert(data.end(), second.begin(), second.end());

    auto frames = p.feed(data);
    ASSERT_EQ(frames.size(), 2u);
    EXPECT_EQ(frames[0].payload, (Bytes{1}));
    EXPECT_EQ(frames[1].payload, (Bytes{2, 3}));
}

TEST(FrameParserTest, GarbageBeforeFrameIsIgnored) {
    serial::FrameParser p;
    Bytes data{0x00, 0x12, 0xFF};
    Bytes frame = make_frame({7});
    data.insert(data.end(), frame.begin(), frame.end());

    auto frames = p.feed(data);
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].payload, (Bytes{7}));
}

TEST(FrameParserTest, BadChecksumIsDropped) {
    serial::FrameParser p;
    Bytes f = make_frame({1, 2});
    f.back() ^= 0xFF;  // rusak checksum-nya
    EXPECT_TRUE(p.feed(f).empty());
}

TEST(FrameParserTest, EmptyPayload) {
    serial::FrameParser p;
    auto frames = p.feed(make_frame({}));
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_TRUE(frames[0].payload.empty());
}


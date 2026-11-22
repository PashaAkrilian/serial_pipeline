#include <gtest/gtest.h>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "serial/logger.hpp"

namespace {

std::filesystem::path temp_log(const std::string& name) {
    auto p = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove(p);
    return p;
}

}  // namespace

TEST(LoggerTest, WritesFormattedLine) {
    auto path = temp_log("serial_logger_format.log");
    {
        serial::Logger logger(path.string());
        logger.log_frame({1, 2, 3});
    }
    std::ifstream in(path);
    std::string line;
    ASSERT_TRUE(std::getline(in, line));
    EXPECT_NE(line.find("len=3"), std::string::npos);
    EXPECT_NE(line.find("01 02 03"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(LoggerTest, ThrowsOnBadPath) {
    EXPECT_THROW(serial::Logger("/nonexistent_dir/x.log"), std::runtime_error);
}

TEST(LoggerTest, ConcurrentWritesKeepLinesIntact) {
    auto path = temp_log("serial_logger_concurrent.log");
    {
        serial::Logger logger(path.string());
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&logger] {
                for (int i = 0; i < 250; ++i) logger.log_frame({0xDE, 0xAD, 0xBE, 0xEF});
            });
        }
        for (auto& th : threads) th.join();
    }
    std::ifstream in(path);
    std::string line;
    int count = 0;
    while (std::getline(in, line)) {
        ++count;
        size_t pos = line.find(" | len=");
        ASSERT_NE(pos, std::string::npos) << line;
        ASSERT_GT(pos, 0u) << line;
        for (size_t i = 0; i < pos; ++i) ASSERT_TRUE(std::isdigit(static_cast<unsigned char>(line[i]))) << line;
    }
    EXPECT_EQ(count, 1000);
    std::filesystem::remove(path);
}

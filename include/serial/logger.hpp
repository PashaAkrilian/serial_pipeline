#pragma once
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace serial {

class Logger {
public:
    explicit Logger(const std::string& path) : out_(path, std::ios::app) {
        if (!out_) {
            throw std::runtime_error("Gagal membuka file log: " + path);
        }
    }

    // format: <epoch_ms> | len=<N> | <hex dipisah spasi>
    void log_frame(const std::vector<std::uint8_t>& payload) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch()).count();
        std::string line = std::to_string(ms) + " | len=" + std::to_string(payload.size()) + " | ";
        char hex[4];
        for (std::size_t i = 0; i < payload.size(); ++i) {
            std::snprintf(hex, sizeof(hex), "%02X", payload[i]);
            if (i > 0) line += ' ';
            line += hex;
        }
        line += '\n';

        std::lock_guard<std::mutex> lock(mutex_);
        out_ << line;
        out_.flush();
    }

private:
    std::ofstream out_;
    std::mutex mutex_;
};

}  // namespace serial

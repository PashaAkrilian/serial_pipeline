#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace serial {

class SerialPort {
public:
    SerialPort(const std::string& device, int baud_rate);
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    std::size_t read(std::uint8_t* buffer, std::size_t max_len);

private:
    int fd_ = -1;
};

}  // namespace serial
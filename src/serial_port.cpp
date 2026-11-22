#include "serial/serial_port.hpp"

#include <cerrno>
#include <stdexcept>
#include <system_error>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace serial {

namespace {

speed_t to_speed(int baud) {
    switch (baud) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        default:
            throw std::invalid_argument("Unsupported baud rate");
    }
}

}  // namespace

SerialPort::SerialPort(const std::string& device, int baud_rate) {
    speed_t speed = to_speed(baud_rate);

    fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ < 0) {
        throw std::system_error(errno, std::generic_category(), "open " + device);
    }

    termios tty{};
    if (tcgetattr(fd_, &tty) != 0) {
        int err = errno;
        ::close(fd_);
        throw std::system_error(err, std::generic_category(), "tcgetattr");
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    tty.c_cflag |= (CLOCAL | CREAD);
    // timeout 100 ms: read() kembali 0 berkala sehingga flag berhenti selalu dicek
    // (menutup race SIGINT); port dicabut tetap muncul sebagai error EIO.
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        int err = errno;
        ::close(fd_);
        throw std::system_error(err, std::generic_category(), "tcsetattr");
    }

    tcflush(fd_, TCIOFLUSH);
}

SerialPort::~SerialPort() {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

std::size_t SerialPort::read(std::uint8_t* buffer, std::size_t max_len) {
    ssize_t n = ::read(fd_, buffer, max_len);
    if (n < 0) {
        if (errno == EINTR) {
            return 0;  // dihentikan sinyal (mis. SIGINT), biarkan pemanggil cek flag
        }
        throw std::system_error(errno, std::generic_category(), "read");
    }
    return static_cast<std::size_t>(n);
}

}  // namespace serial
#include <atomic>
#include <csignal>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <utility>

#include "serial/frame_parser.hpp"
#include "serial/logger.hpp"
#include "serial/serial_port.hpp"
#include "serial/thread_pool.hpp"

namespace {

std::atomic<bool> g_running{true};

void on_sigint(int) { g_running = false; }

}  // namespace

int main(int argc, char* argv[]) {
    std::string device = (argc > 1) ? argv[1] : "/dev/ttyUSB0";
    std::string log_path = (argc > 2) ? argv[2] : "frames.log";

    // tanpa SA_RESTART supaya read() terputus dengan EINTR saat Ctrl+C
    struct sigaction sa{};
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    try {
        serial::Logger logger(log_path);
        serial::SerialPort port(device, 115200);
        serial::FrameParser parser;
        serial::ThreadPool pool(4);

        std::uint8_t buf[256];
        std::cout << "Membaca dari " << device << ", log ke " << log_path
                  << " (Ctrl+C untuk berhenti)\n";
        while (g_running) {
            std::size_t n = port.read(buf, sizeof(buf));
            if (n == 0) continue;
            for (auto& frame : parser.feed(buf, n)) {
                pool.submit([frame = std::move(frame), &logger]() {
                    logger.log_frame(frame.payload);
                });
            }
        }

        std::cout << "Menghentikan pipeline...\n";
        pool.shutdown();
        std::cout << "Selesai.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

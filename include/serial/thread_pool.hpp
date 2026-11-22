#pragma once
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>
#include "serial/thread_safe_queue.hpp"

namespace serial {

class ThreadPool {
public:
    using Task = std::function<void()>;

    explicit ThreadPool(std::size_t num_threads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void submit(Task task);

    // Aman dipanggil berkali-kali
    void shutdown();

private:
    void worker_loop();

    ThreadSafeQueue<Task> tasks_;
    std::vector<std::thread> workers_;
};

}  // namespace serial
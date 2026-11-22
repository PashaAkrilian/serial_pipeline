#include "serial/thread_pool.hpp"

#include <utility>

namespace serial {

ThreadPool::ThreadPool(std::size_t num_threads) {
    workers_.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::submit(Task task) {
    tasks_.push(std::move(task));
}

void ThreadPool::shutdown() {
    tasks_.close();
    for (auto& w : workers_) {
        if (w.joinable()) {
            w.join();
        }
    }
}

void ThreadPool::worker_loop() {
    while (auto task = tasks_.pop()) {
        try {
            (*task)();
        } catch (...) {
            // Exception dari satu tugas tidak boleh mematikan worker
        }
    }
}

}  // namespace serial
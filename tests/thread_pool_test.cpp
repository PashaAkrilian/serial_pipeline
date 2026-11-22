#include <gtest/gtest.h>
#include <atomic>
#include <stdexcept>
#include "serial/thread_pool.hpp"

TEST(ThreadPoolTest, RunsAllSubmittedTasks) {
    std::atomic<int> counter{0};
    serial::ThreadPool pool(4);
    for (int i = 0; i < 100; ++i) {
        pool.submit([&counter] { ++counter; });
    }
    pool.shutdown();
    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, DestructorWaitsForQueuedTasks) {
    std::atomic<int> counter{0};
    {
        serial::ThreadPool pool(2);
        for (int i = 0; i < 50; ++i) {
            pool.submit([&counter] { ++counter; });
        }
    }
    EXPECT_EQ(counter.load(), 50);
}

TEST(ThreadPoolTest, SubmitAfterShutdownIsIgnored) {
    std::atomic<int> counter{0};
    serial::ThreadPool pool(2);
    pool.shutdown();
    pool.submit([&counter] { ++counter; });
    pool.shutdown();  // aman dipanggil dua kali
    EXPECT_EQ(counter.load(), 0);
}

TEST(ThreadPoolTest, ThrowingTaskDoesNotKillPool) {
    std::atomic<int> counter{0};
    serial::ThreadPool pool(1);
    pool.submit([] { throw std::runtime_error("boom"); });
    pool.submit([&counter] { ++counter; });
    pool.shutdown();
    EXPECT_EQ(counter.load(), 1);
}
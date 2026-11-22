#include <gtest/gtest.h>
#include <thread>
#include "serial/thread_safe_queue.hpp"

TEST(ThreadSafeQueueTest, PreservesFifoOrder) {
    serial::ThreadSafeQueue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    EXPECT_EQ(q.pop(), 1);
    EXPECT_EQ(q.pop(), 2);
    EXPECT_EQ(q.pop(), 3);
}

TEST(ThreadSafeQueueTest, PushAfterCloseIsIgnored) {
    serial::ThreadSafeQueue<int> q;
    q.close();
    q.push(5);
    EXPECT_EQ(q.size(), 0u);
    EXPECT_FALSE(q.pop().has_value());
}

TEST(ThreadSafeQueueTest, CloseWakesBlockedConsumer) {
    serial::ThreadSafeQueue<int> q;
    std::thread consumer([&] { EXPECT_FALSE(q.pop().has_value()); });
    q.close();
    consumer.join();
}

TEST(ThreadSafeQueueTest, ProducerConsumerDeliversEveryItem) {
    serial::ThreadSafeQueue<int> q;
    constexpr int kCount = 1000;

    std::thread producer([&] {
        for (int i = 0; i < kCount; ++i) q.push(i);
        q.close();
    });

    long long sum = 0;
    int received = 0;
    while (auto v = q.pop()) {
        sum += *v;
        ++received;
    }
    producer.join();

    EXPECT_EQ(received, kCount);
    EXPECT_EQ(sum, static_cast<long long>(kCount) * (kCount - 1) / 2);
}
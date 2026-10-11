/// @file test_thread_pool.cpp
/// @brief Unit tests for ThreadPool, future resolution, and exception propagation.

#include <gtest/gtest.h>

#include <atomic>
#include <vector>

#include "optiplan/thread_pool.hpp"

using namespace optiplan;

TEST(ThreadPoolTest, SubmitAndResolveFuture) {
    ThreadPool pool(4);

    auto f1 = pool.submit([] { return 42; });
    auto f2 = pool.submit([](int a, int b) { return a + b; }, 10, 20);

    EXPECT_EQ(f1.get(), 42);
    EXPECT_EQ(f2.get(), 30);
}

TEST(ThreadPoolTest, ConcurrentTaskExecution) {
    constexpr int num_tasks = 100;
    ThreadPool pool(8);
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;

    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.submit([&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    EXPECT_EQ(counter.load(), num_tasks);
}

TEST(ThreadPoolTest, ExceptionPropagationThroughFuture) {
    ThreadPool pool(2);

    auto f = pool.submit([]() -> int {
        throw std::runtime_error("simulated failure inside thread worker");
    });

    EXPECT_THROW(f.get(), std::runtime_error);
}

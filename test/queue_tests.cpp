//
// Created by Martin Boros on 12/27/22.
//

#include "lock_free_queue.h"
#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <atomic>
#include <string>

TEST(QueueTest, size) {
    LockFreeQueue<int> q;
    for (int i = 0; i < 100; ++i) {
        q.push(i);
    }

    ASSERT_EQ(100, q.size());
}

TEST(QueueTest, runWithConcurrentConsumersProducers) {
    const int THREADS = 20;

    const int NUMS = 100000;

    const int SENTINEL = -1;
    LockFreeQueue<int> q;

    std::vector<std::atomic<int>> countEachNumber(NUMS);

    std::vector<std::thread> producers;

    for (int i = 0; i < THREADS; ++i) {
        producers.emplace_back([&q] {
            for (int j = 0; j < NUMS; ++j) {
                q.push(j);
            }
            q.push(SENTINEL);
        });
    }
    std::atomic<int> sentinelCount = 0;

    std::mutex m;
    std::condition_variable cv;

    std::vector<std::thread> consumers;
    for (int i = 0; i < THREADS; ++i) {
        consumers.emplace_back([&q, &sentinelCount, &countEachNumber, &cv, &m] {
            int value;
            while (sentinelCount != THREADS) {
                if (q.pop(value)) {
                    if (value == SENTINEL) {
                        sentinelCount++;
                        cv.notify_all();
                    } else {
                        countEachNumber.at(value)++;
                    }
                }
            }
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock, [&sentinelCount] { return sentinelCount == THREADS; });
        });
    }

    for (auto& t : producers) {
        t.join();
    }
    for (auto& t : consumers) {
        t.join();
    }

    for (const auto& n : countEachNumber) {
        ASSERT_EQ(THREADS, n);
    }
}

TEST(QueueTest, queueWithNonTrivialType) {
    const int THREADS = 20;

    const int NUMS = 100000;

    const std::string SENTINEL = "SENTINEL";
    LockFreeQueue<std::string> q;

    std::vector<std::atomic<int>> countEachNumber(NUMS);

    std::vector<std::thread> producers;

    for (int i = 0; i < THREADS; ++i) {
        producers.emplace_back([&q, &SENTINEL] {
            for (int j = 0; j < NUMS; ++j) {
                q.push(std::to_string(j));
            }
            q.push(SENTINEL);
        });
    }
    std::atomic<int> sentinelCount = 0;

    std::mutex m;
    std::condition_variable cv;

    std::vector<std::thread> consumers;
    for (int i = 0; i < THREADS; ++i) {
        consumers.emplace_back([&q, &sentinelCount, &countEachNumber, &cv, &m, &SENTINEL] {
            std::string value;
            while (sentinelCount != THREADS) {
                if (q.pop(value)) {
                    if (value == SENTINEL) {
                        sentinelCount++;
                        cv.notify_all();
                    } else {
                        countEachNumber.at(atoi(value.c_str()))++;
                    }
                }
            }
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock, [&sentinelCount] { return sentinelCount == THREADS; });
        });
    }

    for (auto& t : producers) {
        t.join();
    }
    for (auto& t : consumers) {
        t.join();
    }

    for (const auto& n : countEachNumber) {
        ASSERT_EQ(THREADS, n);
    }

}
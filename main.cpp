#include <iostream>
#include <thread>
#include <random>
#include "lock_free_queue.h"

const int THREADS = 20;

const int NUMS = 100000;

const int SENTINEL = -1;

int main() {
    LockFreeQueue<int> q;

    std::vector<std::thread> producers;

    for (int i = 0; i < THREADS; ++i) {
        producers.emplace_back([&q] {
            for (int j = 0; j < NUMS; ++j) {
                q.push(j);
            }
            q.push(SENTINEL);
        });
    }

    std::atomic<size_t> accumulator = 0;
    std::atomic<int> sentinelCount = 0;

    std::vector<std::thread> consumers;
    for (int i = 0; i < THREADS; ++i) {
        consumers.emplace_back([&q, &sentinelCount, &accumulator] {
            int value;
            while (sentinelCount != THREADS) {
                if (q.pop(value)) {
                    if (value == SENTINEL) {
                        sentinelCount++;
                    } else {
                        accumulator += value;
                    }
                }
            }
        });
    }


    for (auto& t : producers) {
        t.join();
    }
    for (auto& t : consumers) {
        t.join();
    }


    std::cout << "accumulator actual: " << accumulator << '\n';


}


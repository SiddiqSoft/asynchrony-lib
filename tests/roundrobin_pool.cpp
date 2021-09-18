/*
    asynchrony-lib
    Add asynchrony to your apps

    BSD 3-Clause License

    Copyright (c) 2021, Siddiq Software LLC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gtest/gtest.h"

#include <iostream>
#include <format>
#include <string>
#include <barrier>
#include <vector>

#include "nlohmann/json.hpp"
#include "../src/roundrobin_pool.hpp"


TEST(roundrobin_pool, test1)
{
    std::atomic_uint                            passTest {0};

    siddiqsoft::roundrobin_pool<nlohmann::json> workers {[&passTest](auto& item) {
        std::cerr << std::format("Item:{} .. Got object: {}\n", passTest.load(), item.dump());
        passTest++;
    }};

    for (unsigned i = 0; i < std::thread::hardware_concurrency(); i++) {
        workers.queue({{"test", "roundrobin_pool"}, {"hello", "world"}, {"i", i}});
    }

    // This is important otherwise the destructor will kill the thread before it has a chance to process anything!
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(passTest.load(), std::thread::hardware_concurrency());
    std::cerr << nlohmann::json(workers).dump() << std::endl;
}


TEST(roundrobin_pool, test2)
{
    constexpr auto            FEEDER_COUNT = 8;
    std::barrier              startFeeding {FEEDER_COUNT};
    constexpr auto            WORKER_POOLSIZE = 8 * FEEDER_COUNT;
    std::atomic_uint          passTest {0};
    std::vector<std::jthread> feeders {};

    // The target is our workers
    siddiqsoft::roundrobin_pool<nlohmann::json, WORKER_POOLSIZE> workers {[&passTest](auto& item) {
        std::cerr << std::this_thread::get_id() << std::format("..Item {:03} .. Got object: {}\n", passTest.load(), item.dump());
        passTest++;
    }};

    // This is our thread pool that will try to inject into the workers all at the same time
    for (auto f = 0; f < FEEDER_COUNT; f++) {
        feeders.emplace_back([&]() {
            // Each thread waits
            startFeeding.arrive_and_wait();
            // ..until everyone's "arrived" and then they all simultaneously should feed into the workers
            // with the objective that we excercise the correctness and shake out any potential race condition
            for (auto j = 0; j < WORKER_POOLSIZE; j++) {
                workers.queue({{"test", "roundrobin_pool"}, {"hello", "world"}, {"j", j}});
            }
        });
    }

    // This is important otherwise the destructor will kill the thread before it has a chance to process anything!
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(passTest.load(), FEEDER_COUNT * WORKER_POOLSIZE);
}

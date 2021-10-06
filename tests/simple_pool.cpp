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
#include <thread>
#include <barrier>

#include "nlohmann/json.hpp"
#include "../src/simple_pool.hpp"

TEST(simple_pool, test1)
{
    std::atomic_uint                        passTest {0};

    siddiqsoft::simple_pool<nlohmann::json> workers {[&passTest](auto& item) {
        std::cerr << std::format("Item:{} .. Got object: {}\n", passTest.load(), item.dump());
        passTest++;
    }};

    for (unsigned i = 0; i < std::thread::hardware_concurrency(); i++) {
        workers.queue({{"test", "simple_pool"}, {"hello", "world"}, {"i", i}});
    }

    // This is important otherwise the destructor will kill the thread before it has a chance to process anything!
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(passTest.load(), std::thread::hardware_concurrency());
    std::cerr << nlohmann::json(workers).dump() << std::endl;
}


TEST(simple_pool, test2)
{
    const auto                FEEDER_COUNT = 2;
    std::barrier              startFeeding {FEEDER_COUNT};
    constexpr auto            WORKER_POOLSIZE = 2 * FEEDER_COUNT;
    std::atomic_uint          passTest {0};
    std::vector<std::jthread> feeders {};

    // The target is our workers
    siddiqsoft::simple_pool<nlohmann::json, WORKER_POOLSIZE> workers {[&passTest](auto& item) {
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
                workers.queue({{"test", "simple_pool"}, {"hello", "world"}, {"p2j", std::to_string(j)}});
            }
        });
    }

    // This is important otherwise the destructor will kill the thread before it has a chance to process anything!
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(passTest.load(), FEEDER_COUNT * WORKER_POOLSIZE);
}


/*
 * The test is to check if we have the proper capability to handle default copy/move constructors/operators
 * for the simple_pool object. See the issue: https://github.com/SiddiqSoft/asynchrony-lib/issues/3
 */


class meow_type : public nlohmann::json
{
public:
    static const inline std::string L1 = "meow_type";

    meow_type(const nlohmann::json& src)
    {
        this->update(src);
    }

    meow_type(nlohmann::json&& src)
    {
        this->swap(src);
        // static_cast<nlohmann::json>(*this).operator=(std::move(src));
    }

    meow_type(meow_type&&) = default;


    meow_type(const meow_type& src)
    {
        this->update(nlohmann::json(src));
    }

    meow_type& operator=(meow_type&& src) = default;


    meow_type& operator                   =(nlohmann::json&& src)
    {
        this->swap(src);
        return *this;
    }
};


struct cat_type
{
    cat_type(nlohmann::json&& src, std::string&& n)
        : meow(std::move(src))
        , name(std::move(n))
    {
    }
    meow_type   meow;
    std::string name {};
};


TEST(simple_pool, test3)
{
    std::atomic_uint                  passTest {0};


    siddiqsoft::simple_pool<cat_type> workers {[&passTest](auto& item) {
        std::cerr << std::format("Item:{} .. Got object: >{} -- {}<\n", passTest.load(), item.meow.dump(), item.name);
        passTest++;
    }};

    for (unsigned i = 0; i < std::thread::hardware_concurrency(); i++) {
        try {
            workers.queue(
                    cat_type(nlohmann::json {{"test", "simple_pool"}, {"hello", "world"}, {"i", i}}, std::format("test3..{}", i)));
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    // This is important otherwise the destructor will kill the thread before it has a chance to process anything!
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    EXPECT_EQ(passTest.load(), std::thread::hardware_concurrency());
    std::cerr << nlohmann::json(workers).dump() << std::endl;
}

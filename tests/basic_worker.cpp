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

#include "nlohmann/json.hpp"
#include "../src/basic_worker.hpp"


TEST(basic_worker, test1)
{
    bool                                     passTest {false};

    siddiqsoft::basic_worker<nlohmann::json> worker {[&](auto& item) {
        std::cerr << std::format("Got object: {}\n", item.dump());
        passTest = true;
    }};

    worker.queue({{"hello", "world"}});

    // This is important otherwise the destructor will kill the thread before it has a chance to process anything!
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_TRUE(passTest);
}


TEST(basic_worker, test2)
{
    bool                                                      passTest {false};

    siddiqsoft::basic_worker<std::shared_ptr<nlohmann::json>> worker {[&](auto& item) {
        std::cerr << std::format("Got object: {}\n", item->dump());
        passTest = true;
    }};

    worker.queue(std::make_shared<nlohmann::json>(nlohmann::json {{"hello", "world"}}));

    // This is important otherwise the destructor will kill the thread before it has a chance to process anything!
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_TRUE(passTest);
}


TEST(basic_worker, test3)
{
    bool passTest {false};

    struct nonCopyableObject
    {
        std::string Data {};

        nonCopyableObject(const std::string& s)
            : Data(s)
        {
        }

        nonCopyableObject(nonCopyableObject&) = delete;
        nonCopyableObject& operator=(nonCopyableObject&) = delete;

        // Move constructors
        nonCopyableObject(nonCopyableObject&&) = default;
        nonCopyableObject& operator=(nonCopyableObject&&) = default;
    };

    siddiqsoft::basic_worker<nonCopyableObject> worker {[&](auto& item) {
        std::cerr << std::format("Got object: {}\n", item.Data);
        passTest = true;
    }};

    worker.queue(nonCopyableObject { "Hello world!"});

    // This is important otherwise the destructor will kill the thread before it has a chance to process anything!
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_TRUE(passTest);
}

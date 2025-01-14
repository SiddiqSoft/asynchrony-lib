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
#include "../include/siddiqsoft/resource_pool.hpp"


TEST(resource_pool, T_int)
{
    bool passTest {false};

    EXPECT_NO_THROW({
        siddiqsoft::resource_pool<int> rp {};
        std::cerr << std::format("{} - Capacity:{}\n", __func__, rp.size());
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

TEST(resource_pool, T_shared_ptr_string)
{
    bool passTest {false};

    EXPECT_NO_THROW({
        siddiqsoft::resource_pool<std::shared_ptr<std::string>> rp {};

        EXPECT_EQ(0, rp.size());
        rp.checkin(std::shared_ptr<std::string>(new std::string(__TIME__)));
        EXPECT_EQ(1, rp.size());

        auto item = rp.checkout();
        EXPECT_EQ(0, rp.size());
        EXPECT_EQ(__TIME__, *item);
        (*item).append("-ok");

        rp.checkin(std::move(item));
        EXPECT_EQ(1, rp.size());

        auto item2 = rp.checkout();
        EXPECT_EQ(0, rp.size());
        EXPECT_TRUE(item2->ends_with("-ok"));

        rp.checkin(std::move(item2));
        EXPECT_EQ(1, rp.size());

        passTest = true;
    });

    EXPECT_TRUE(passTest);
}


TEST(resource_pool, T_unique_ptr_string)
{
    bool passTest {false};

    EXPECT_NO_THROW({
        siddiqsoft::resource_pool<std::unique_ptr<std::string>> rp {};

        EXPECT_EQ(0, rp.size());
        rp.checkin(std::unique_ptr<std::string>(new std::string(__TIME__)));
        EXPECT_EQ(1, rp.size());

        auto item = rp.checkout();
        EXPECT_EQ(0, rp.size());
        EXPECT_EQ(__TIME__, *item);
        (*item).append("-ok");

        rp.checkin(std::move(item));
        EXPECT_EQ(1, rp.size());

        auto item2 = rp.checkout();
        EXPECT_EQ(0, rp.size());
        EXPECT_TRUE(item2->ends_with("-ok"));

        rp.checkin(std::move(item2));
        EXPECT_EQ(1, rp.size());

        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

TEST(resource_pool, T_checkin_checkout_unique_ptr_string)
{
    bool passTest {false};

    EXPECT_NO_THROW({
        siddiqsoft::resource_pool<std::unique_ptr<std::string>> rp {};

        EXPECT_EQ(0, rp.size());
        rp.checkin(std::unique_ptr<std::string>(new std::string(__TIME__)));
        EXPECT_EQ(1, rp.size());

        // Immediately push back in.. we're testing to make sure that there is
        // no leakage!
        rp.checkin(rp.checkout());
        EXPECT_EQ(1, rp.size());

        auto item2 = rp.checkout();
        EXPECT_EQ(0, rp.size());
        EXPECT_EQ(__TIME__, *item2);

        passTest = true;
    });

    EXPECT_TRUE(passTest);
}



TEST(resource_pool, T_checkin_checkout_vector_string)
{
    bool passTest {false};

    EXPECT_NO_THROW({
        siddiqsoft::resource_pool<std::vector<std::string>> rp {};

        EXPECT_EQ(0, rp.size());
        rp.checkin({"A", "B", "C"});
        EXPECT_EQ(1, rp.size());

        // Immediately push back in.. we're testing to make sure that there is
        // no leakage!
        rp.checkin(rp.checkout());
        EXPECT_EQ(1, rp.size());

        auto item2 = rp.checkout();
        item2.push_back("1");
        item2.push_back("2");
        item2.push_back("3");
        EXPECT_EQ(0, rp.size());
        EXPECT_EQ(6, item2.size());

        rp.checkin(std::move(item2));
        // The resource is now empty; the checking moved now owns it.
        EXPECT_EQ(0, item2.size());

        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

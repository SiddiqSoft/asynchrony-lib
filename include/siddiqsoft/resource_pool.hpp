/*
    asynchrony : Add asynchrony to your apps

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
       this software without specific Szor written permission.

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

#pragma once
#include <stdexcept>
#ifndef RESOURCE_POOL_HPP
#define RESOURCE_POOL_HPP

#include <mutex>
#include <shared_mutex>
#include <deque>

#include "siddiqsoft/RunOnEnd.hpp"

namespace siddiqsoft
{
    /**
     * @brief Implements a resource pool that stores objects of type T.
     *        Said objects can be shared_ptr or unique_ptr
     *        Client "acquire" and "release" T from this pool.
     * @tparam T The storage element type. Maybe shared_ptr or unique_ptr
     * @tparam Sz The capacity of the pool. If this clients continually starve
     *            then this value must be increased. Usually a perfect number
     *            is the core-count on your host.
     */
    template <typename T>
        requires std::move_constructible<T>
    class resource_pool
    {
    private:
        std::deque<T>        _pool {};
        std::recursive_mutex _poolLock {};

    public:
        resource_pool()                               = default;
        resource_pool(resource_pool&)                 = delete;
        resource_pool(resource_pool&& src)            = default;
        resource_pool& operator=(resource_pool&)      = delete;
        resource_pool& operator=(resource_pool&& src) = default;

        ~resource_pool()
        {
            if (std::lock_guard<std::recursive_mutex> l(_poolLock); !_pool.empty()) {
                _pool.clear();
            }
        }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
#endif

        auto getCapacity()
        {
            if (std::lock_guard<std::recursive_mutex> l(_poolLock); !_pool.empty()) {
                return _pool.size();
            }

            return size_t(0);
        }

        [[nodiscard]] T&& checkout() /* throw() */
        {
            if (std::lock_guard<std::recursive_mutex> l(_poolLock); !_pool.empty()) {
                RunOnEnd roe([&]() { _pool.pop_front(); });
                return std::move(_pool.front());
            }

            throw std::runtime_error("Empty pool; add something first!");
        }

        /**
         * @brief Insert a new element or return a borrowed element
         *
         * @param rsrc R-Value for the item to return to the pool (previously checkout'd or create a new one!)
         */
        void checkin(T&& rsrc)
        {
            if (std::lock_guard<std::recursive_mutex> l(_poolLock); true) {
                _pool.push_back(std::move(rsrc));
            }
        }
    };
} // namespace siddiqsoft
#endif

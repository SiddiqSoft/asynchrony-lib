/*
    roundrobin pool : Add asynchrony to your apps

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

#pragma once
#ifndef ROUNDROBIN_POOL_HPP
#define ROUNDROBIN_POOL_HPP

#include "simple_worker.hpp"


namespace siddiqsoft
{
    /// @brief Implements a lock-free round robin work allocation into vector of simple_worker<T>
    /// @tparam T Your datatype
    /// #tparam N Number of threads in the pool. Leave it to 0 to use the value returned by std::thread::hardware_concurrency()
    /// @remarks The number of threads in the pool is determined by the nature of your "work". If you're spending time against db
    /// then you might wish to use more threads as individual queries might take time and hog the thread.
    template <typename T, uint16_t N = 0>
        requires std::move_constructible<T>
    struct roundrobin_pool
    {
    public:
        roundrobin_pool(roundrobin_pool&&) = delete;
        auto operator=(roundrobin_pool&&) = delete;
        roundrobin_pool(roundrobin_pool&) = delete;
        auto operator=(roundrobin_pool&) = delete;


        /// @brief Consturcts a vector of simple_worker<T> with the given callback
        /// @param c Callback worker function
        roundrobin_pool(std::function<void(T&&)> c)
        {
            // *CRITICAL*
            // This is step is *critical* otherwise we will end up moving threads as we add elements to the vector.
            workers.reserve(N > 0 ? N : std::thread::hardware_concurrency());

            // Create as many threads as reported by the system..
            for (unsigned i = 0; i < ((N > 0) ? N : std::thread::hardware_concurrency()); i++) {
                workers.emplace_back(c);
            }

            // Shortcut; save the size of the array
            workersSize = workers.size();
        }

        /// @brief Queue item into one of the thread's queue.
        /// @param item The item must be std::move'd
        /// @remarks The choice of the roundrobin might mean that if one thread is blocked then the rest of the items for this queue
        /// would be blocked. The cost of the % is cheaper than the cost it takes to pay for locks. The round-robin approach ensures
        /// that the underlying deque isn't being pop'd and push'd by multiple threads as there is only one consumer for that deque
        /// (one per thread) while we may have any number of producers.
        void queue(T&& item)
        {
            // Increment counter *before* we invoke nextWorkerIndex..
            ++queueCounter;
            // Add into the thread's internal queue
            workers.at(nextWorkerIndex()).queue(std::move(item));
        }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        /// @brief Serializer for json
        /// @param  destination
        /// @param  this object
        nlohmann::json toJson() const
        {
            return {{"_typver", "siddiqsoft.asynchrony-lib.roundrobin_pool/0.10"},
                    {"workersSize", workersSize},
                    {"queueCounter", queueCounter.load()}};
        }
#endif

#ifdef _DEBUG
    public:
        std::atomic_uint64_t queueCounter {0};
#else
    private:
        std::atomic_uint64_t queueCounter {0};
#endif

    private:
        /// @brief Vector of the simple_worker elements of type T
        std::vector<simple_worker<T>> workers {};

        /// @brief Tracks the size of the array workers
        uint64_t workersSize {};

        /// @brief Calculates the index into the workers thread using modulo and the running counter of the number of items pushed
        /// into the queue.
        /// @return size_t index into the workers array
        constexpr size_t nextWorkerIndex()
        {
            return workersSize == 0 ? 0 : queueCounter.load() % workersSize;
        }
    };

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
    /// @brief Serializer for the roundrobin_pool
    /// @tparam T base typename
    /// @param dest destination json object
    /// @param src source object
    template <typename T, uint16_t N = 0>
    static void to_json(nlohmann::json& dest, const siddiqsoft::roundrobin_pool<T, N>& src)
    {
        dest = src.toJson();
    }
#endif

} // namespace siddiqsoft
#endif // !BASIC_WORKER_HPP

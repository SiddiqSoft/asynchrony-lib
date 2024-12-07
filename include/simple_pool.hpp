/*
    basic-pool : Add asynchrony to your apps

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
#ifndef SIMPLE_POOL_HPP
#define SIMPLE_POOL_HPP

#include "simple_worker.hpp"
#include <optional>
#include <latch>

namespace siddiqsoft
{
    /// @brief Implements a single deque based vector of jthreads. All threads wait on the next available item via semaphore and
    /// invoke the callback on the next item from the deque. Items in the deque are lock-accessed.
    /// @tparam T Your datatype
    /// #tparam N Number of threads in the pool. Leave it to 0 to use the value returned by std::thread::hardware_concurrency()
    /// @remarks The number of threads in the pool is determined by the nature of your "work". If you're spending time against db
    /// then you might wish to use more threads as individual queries might take time and hog the thread.
    template <typename T, uint16_t N = 0>
        requires std::move_constructible<T>
    struct simple_pool
    {
        simple_pool(simple_pool&&)            = delete;
        simple_pool& operator=(simple_pool&&) = delete;
        simple_pool(simple_pool&)             = delete;
        simple_pool& operator=(simple_pool&)  = delete;


        /// @brief Destructor.
        /// @remarks We need to make sure that the signal wait interval is reduced to 1ms to allow our thread (which are waiting on
        /// the signal) can be stopped.
        ~simple_pool()
        {
            // Reduce the wait interval to ensure that the threads waiting on the signal abort
            signalWaitInterval = std::chrono::milliseconds(0);
            // Compared to skipping the following code, we save at least about 100ms
            // of idle time waiting for the threads to be signalled by default.
            for (auto& t : workers) {
                // Release the signal to indicate to the threads to abandon.
                signal.release();
                // Signal the threads to stop
                if (t.request_stop() && t.joinable()) t.join();
            }
        }


        /// @brief Contructs a threadpool with N threads with the given callback/worker function
        /// @param c The worker function.
        simple_pool(std::function<void(T&&)> c)
            : callback(c)
        {
            // *CRITICAL*
            // This is step is *critical* otherwise we will end up moving threads as we add elements to the vector.
            workers.reserve((N > 0) ? N : std::thread::hardware_concurrency());

            // Create as many threads as reported by the system..
            for (unsigned i = 0; i < ((N > 0) ? N : std::thread::hardware_concurrency()); i++) {
                // Add the thread with the main driver
                workers.emplace_back([&](std::stop_token st) {
                    // The driver runs forever until signalled to stop
                    // Tries to get next item ready in the queue (for max 1s cycle)
                    // If we have an item, invoke the callback with the item
                    while (!st.stop_requested()) {
                        try {
                            // The getNextItem performs the wait on the signal and if it expires, returns empty.
                            // If there is an item, it will get that item (minimizing move) and performs the pop
                            // and returns the item so we can invoke the callback outside the lock.
                            if (auto item = getNextItem(signalWaitInterval); item && !st.stop_requested()) {
                                // Delegate to the callback outside the lock
                                callback(std::move(*item));
                            }
                        }
                        catch (...) {
                        }
                    } // while ..continue until we're asked to stop
                });
            }
        }

        /// @brief Queue item into the deque (takes "ownership" of the item)
        /// @param item Item to queue must be move'd
        void queue(T&& item)
        {
            {
                std::unique_lock<std::shared_mutex> myWriterLock(items_mutex);

                items.emplace_back(std::move(item));
            }
            signal.release();
            ++queueCounter;
        }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        /// @brief Serializer for json
        /// @param  destination
        /// @param  this object
        /// @note The use of signal.max() is causing an issue where winmindef.h is defining the `max` as a macro and thus we end up
        /// with compiler error when the client application includes any of the windows headers! Disabled for now.
        nlohmann::json toJson() const
        {
            return nlohmann::json {{"_typver", "siddiqsoft.asynchrony-lib.simple_pool/0.10"},
                                   {"workersSize", workers.size()},
                                   {"dequeSize", items.size()},
                                   {"queueCounter", queueCounter.load()},
                                   {"waitInterval", signalWaitInterval.count()}};
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
        std::vector<std::jthread> workers {};
        std::function<void(T&&)>  callback;
        std::counting_semaphore<> signal {0};
        std::deque<T>             items {};
        std::shared_mutex         items_mutex;
        /// @brief This is the interval we wait on the signal. It starts off with 500ms and when the thread is to shutdown, it is
        /// set to 1ms.
        std::chrono::milliseconds signalWaitInterval {1500};


        /// @brief Performs an acquire on the semaphore and if successful,
        /// attempts to lock to pull the item from the top of the deque.
        /// @param delta Amount of milliseconds to wait on the semaphore
        /// @return An optional which may contain the item or empty (most of the time it'll be empty)
        std::optional<T> getNextItem(std::chrono::milliseconds& delta)
        {
            if (signal.try_acquire_for(signalWaitInterval)) {
                // Guard against empty signals which are terminating indicator
                std::unique_lock<std::shared_mutex> myWriterLock(items_mutex);
                // Guard against empty signals which are terminating indicator
                if (!items.empty()) {
                    // WE require that the stored type by move-constructible!
                    T item {std::move(items.front())};
                    items.pop_front();
                    return std::move(item);
                }
            }

            // Fall-through empty
            return {};
        }
    };

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
    /// @brief Serializer for the simple_worker
    /// @tparam T base typename
    /// @param dest destination json object
    /// @param src source object
    template <typename T, uint16_t N = 0>
    static void to_json(nlohmann::json& dest, const siddiqsoft::simple_pool<T, N>& src)
    {
        dest = src.toJson();
    }
#endif

} // namespace siddiqsoft
#endif // !BASIC_WORKER_HPP

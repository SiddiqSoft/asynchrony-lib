/*
    asynchrony-lib : Add asynchrony to your apps

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
#ifndef PERIODIC_WORKER_HPP
#define PERIODIC_WORKER_HPP


#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <deque>
#include <semaphore>
#include <stop_token>


namespace siddiqsoft
{
    /// @brief Implements a simple queue + semaphore driven asynchronous processor
    /// @tparam T The data type for this processor
    /// @tparam Pri Optional thread priority level. 0=Normal
    template <int Pri = 0>
        requires((Pri >= -10) && (Pri <= 10))
    struct periodic_worker
    {
    public:
        periodic_worker(periodic_worker&) = delete;
        auto& operator=(periodic_worker&)  = delete;
        periodic_worker(periodic_worker&&) = delete;
        auto& operator=(periodic_worker&&) = delete;


        /// @brief Destructor
        /// Cancel the semaphore by first resetting the interval to zero.
        /// Signal the semaphore follow that by requesting thread to stop.
        /// This is slightly better than allowing the default destructors to kick in. If we do not reduce the interval time and
        /// signal a release then the timeout might be quite large!
        ~periodic_worker()
        {
            // This is critical step since we wait on the semaphore for a long time (keeps threads suspended) and if we do not
            // decrease this interval then the shutdown will be quite delayed.
            invokePeriod = std::chrono::milliseconds(0);
            // Empty signal to get our thread to wake up
            signal.release();
            try {
                // Ask thread to shutdown
                if (processor.request_stop() && processor.joinable()) processor.join();
            }
            catch (const std::exception&) {
            }
        }


        /// @brief Constructor requires the callback for the thread
        /// @param c The callback which accepts the type T as reference and performs action.
        /// @param interval The interval between each invocation
        periodic_worker(std::function<void()> c, std::chrono::microseconds interval, const std::string& name={"anonymous-periodic-worker"})
            : callback(c)
            , invokePeriod(interval)
            , threadName(name)
        {
        }


#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        /// @brief Serializer for json
        /// @param  destination
        /// @param  this object
        /// @note The use of signal.max() is causing an issue where winmindef.h is defining the `max` as a macro and thus we end up
        /// with compiler error when the client application includes any of the windows headers! Disabled for now.
        nlohmann::json toJson() const
        {
            using namespace std;

            return {{"_typver"s, "siddiqsoft.asynchrony-lib.periodic_worker/0.10"s},
                    {"threadName", threadName},
                    {"invokeCounter"s, invokeCounter},
                    {"threadPriority"s, Pri},
                    {"waitInterval"s, invokePeriod.count()}};
        }
#endif

    private:
        /// @brief Internal name of the worker thread (when supported the thread name displays in the debugger)
        std::string threadName {"anonymous-periodic-worker"};
        /// @brief Track number of times we've invoked the callback
        uint64_t invokeCounter {0};
        /// @brief Semaphore with initial max of 128 items (backlog)
        std::counting_semaphore<1> signal {0};
        /// @brief This is the interval we wait on the signal. It starts off with 500ms and when the thread is to shutdown, it is
        /// set to 1ms.
        std::chrono::microseconds invokePeriod {};
        /// @brief The callback is invoked whenever there is an item in the queue
        std::function<void()> callback;
        /// @brief Processor thread
        /// The driver runs forever until signalled to stop
        /// Tries to get next item ready in the queue (for max 500ms cycle)
        /// If we have an item, invoke the callback with the item
        /// @note
        /// The processor thread captures `this` and access the signal and callback
        std::jthread processor {[&](std::stop_token st) {
#if defined(WIN64) || defined(_WIN64) || defined(WIN32) || defined(_WIN32)
            // Set the thread priority if possible
            if constexpr (Pri != 0) SetThreadPriority(GetCurrentThread(), Pri);
#endif

            while (!st.stop_requested()) {
                try {
                    // This will wait until our period and return.
                    // We do not care about the return from try_acquire_for..
                    // We're using it as an efficient "wait" facility for period.
                    auto _ = signal.try_acquire_for(invokePeriod);

                    if (!st.stop_requested()) {
                        // Delegate to the callback outside the lock
                        callback();
                        invokeCounter++;
                    }
                }
                catch (...) {
                }
            } // while ..continue until we're asked to stop
        }};
    };

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
    /// @brief Serializer for the periodic_worker
    /// @tparam T base typename
    /// @param dest destination json object
    /// @param src source object
    template <uint16_t Pri = 0>
    static void to_json(nlohmann::json& dest, const siddiqsoft::periodic_worker<Pri>& src)
    {
        dest = src.toJson();
    }
#endif

} // namespace siddiqsoft
#endif

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
#ifndef BASIC_WORKER_HPP
#define BASIC_WORKER_HPP


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
    template <typename T, uint16_t Pri = 0>
        requires ((Pri >= -10) && (Pri <= 10)) &&
                 std::copy_constructible<T>&&
                 std::move_constructible<T>
    struct basic_worker
    {
        basic_worker(basic_worker&) = delete;
        auto operator=(basic_worker&) = delete;


        ~basic_worker()
        {
            // This is critical step since we wait on the semaphore for a long time (keeps threads suspended) and if we do not
            // decrease this interval then the shutdown will be quite delayed.
            signalWaitInterval = std::chrono::milliseconds(0);
            // Empty signal to get our thread to wake up
            signal.release();
            try {
                // Ask thread to shutdown
                if (processor.request_stop() && processor.joinable()) processor.join();
            }
            catch (const std::exception&) {
            }
        }


        /// @brief Move constructor
        /// @param src Source to be moved into this object
        basic_worker(basic_worker&& src) noexcept
            : callback(std::move(src.callback))
        {
            // NOTE
            // We do not move the items or the signal.. the use case is that we would use the move constructor to facilitate adding
            // this object in std::vector
        }

        /// @brief Constructor requires the callback for the thread
        /// @param c The callback which accepts the type T as reference and performs action.
        basic_worker(std::function<void(T&)> c) noexcept
            : callback(c)
        {
        }


        /// @brief Queue item into this worker thread's deque
        /// @param item This is move'd into the internal deque
        void queue(T&& item)
        {
            {
                std::unique_lock<std::shared_mutex> myWriterLock(items_mutex);

                items.emplace_back(std::move(item));
            }
            // Signal outside the lock and after adding the item.
            signal.release();
        }

    private:
        /// @brief The internal queue for this worker.
        std::deque<T> items {};
        /// @brief Mutex to protect the items
        std::shared_mutex items_mutex {};
        /// @brief Semaphore with initial max of 128 items (backlog)
        std::counting_semaphore<512> signal {0};
        /// @brief This is the interval we wait on the signal. It starts off with 500ms and when the thread is to shutdown, it is
        /// set to 1ms.
        std::chrono::milliseconds signalWaitInterval {1500};
        /// @brief The callback is invoked whenever there is an item in the queue
        std::function<void(T&)> callback;
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
                    if (signal.try_acquire_for(signalWaitInterval)) {
                        // Guard against empty signals which are terminating indicator
                        if (!items.empty() && !st.stop_requested()) {
                            T item;

                            { // scope lock to pull item from the deque
                                std::unique_lock<std::shared_mutex> myWriterLock(items_mutex);
                                item = items.front();
                                items.pop_front();
                            } // end of lock
                            // Delegate to the callback outside the lock
                            callback(item);
                        }
                    }
                }
                catch (...) {
                }
            } // while ..continue until we're asked to stop
        }};
    };
} // namespace siddiqsoft

#endif // !BASIC_WORKER_HPP

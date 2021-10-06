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
#ifndef SIMPLE_WORKER_HPP
#define SIMPLE_WORKER_HPP


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
    /// @brief This helper exists ONLY to ensure that we avoid making a copy and pop item from the deque.
    /// @notes CAUTION. If you do not call pop() then the destructor will
    /// throw away the item when it is invoked.
    /// This structure is private and it is intended to be used as a helper
    /// to ensure we pop the queue item to avoid making copies and invoking
    /// callback inside the lock.
    template <typename T>
        requires std::move_constructible<T>
    struct popOnDestruct
    {
        /// @brief Constructor holds references to the underlying deque and the mutex. When the
        /// @param items The reference to the deque
        /// @param items_mutex The reference to the mutex
        explicit popOnDestruct(std::deque<T>& items, std::shared_mutex& items_mutex) noexcept
            : parentDeque(items)
            , parentMutex(items_mutex)
        {
        }

        /// @brief Returns the item at the front of the deque inside lock
        /// @return Item
        T pop()
        {
            std::unique_lock<std::shared_mutex> myWriterLock(parentMutex);
            return std::move(parentDeque.front());
        }

        /// @brief Pop the front of the queue upon destructor invocation
        ~popOnDestruct() noexcept
        {
            if (!parentDeque.empty()) parentDeque.pop_front();
        }

    private:
        std::deque<T>&     parentDeque; // reference to the parent deque
        std::shared_mutex& parentMutex; // reference to the parent mutex
    };


    /// @brief Implements a simple queue + semaphore driven asynchronous processor
    /// @tparam T The data type for this processor
    /// @tparam Pri Optional thread priority level. 0=Normal
    template <typename T, uint16_t Pri = 0>
        requires((Pri >= -10) && (Pri <= 10))
    &&std::move_constructible<T> struct simple_worker
    {
    public:
        simple_worker(simple_worker&) = delete;
        auto operator=(simple_worker&) = delete;


        ~simple_worker()
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
        simple_worker(simple_worker&& src) noexcept
            : callback(std::move(src.callback))
        {
            // NOTE
            // We do not move the items or the signal.. the use case is that we would use the move constructor to facilitate adding
            // this object in std::vector
        }

        /// @brief Constructor requires the callback for the thread
        /// @param c The callback which accepts the type T as reference and performs action.
        simple_worker(std::function<void(T&)> c)
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
                queueCounter++;
            }
            // Signal outside the lock and after adding the item.
            signal.release();
        }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        /// @brief Serializer for json
        /// @param  destination
        /// @param  this object
        /// @note The use of signal.max() is causing an issue where winmindef.h is defining the `max` as a macro and thus we end up
        /// with compiler error when the client application includes any of the windows headers! Disabled for now.
        nlohmann::json toJson() const
        {
            return {{"_typver", "siddiqsoft.asynchrony-lib.simple_worker/0.10"},
                    {"dequeSize", items.size()},
                    //{"semaphoreMax", signal.max()}, // conflicts with windows headers :-(
                    {"queueCounter", queueCounter},
                    {"threadPriority", Pri},
                    {"waitInterval", signalWaitInterval.count()}};
        }
#endif

    private:
        /// @brief Track number of times we've got items added into our queue
        uint64_t queueCounter {0};
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
                    // The getNextItem performs the wait on the signal and if it expires, returns empty.
                    // If there is an item, it will get that item (minimizing move) and performs the pop
                    // and returns the item so we can invoke the callback outside the lock.
                    if (auto item = getNextItem(signalWaitInterval); item && !st.stop_requested()) {
                        // Delegate to the callback outside the lock
                        callback(*item);
                    }
                }
                catch (...) {
                }
            } // while ..continue until we're asked to stop
        }};

        /// @brief Performs an acquire on the semaphore and if successful,
        /// attempts to lock to pull the item from the top of the deque.
        /// @param delta Amount of milliseconds to wait on the semaphore
        /// @return An optional which may contain the item or empty (most of the time it'll be empty)
        std::optional<T> getNextItem(std::chrono::milliseconds& delta)
        {
            if (signal.try_acquire_for(signalWaitInterval)) {
                // Guard against empty signals which are terminating indicator
                if (!items.empty()) {
                    // Ensures that we pop_front upon exit of this scope
                    popOnDestruct pod {items, items_mutex};
                    // The pop() method gets the front item within lock and gets back the item
                    return pod.pop();
                    // The item is now pop'd due to the destructor invoked by popOnDestruct
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
    template <typename T, uint16_t Pri = 0>
    static void to_json(nlohmann::json& dest, const siddiqsoft::simple_worker<T, Pri>& src)
    {
        dest = src.toJson();
    }
#endif

} // namespace siddiqsoft
#endif // !BASIC_WORKER_HPP

asynchrony : Asynchrony support library
---------------------------------------

<!-- badges -->
[![CodeQL](https://github.com/SiddiqSoft/asynchrony-lib/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/SiddiqSoft/asynchrony-lib/actions/workflows/codeql-analysis.yml)
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.asynchrony-lib?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=17&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.asynchrony-lib)
![](https://img.shields.io/github/v/tag/SiddiqSoft/asynchrony-lib)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/17)
<!--![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/17)-->
<!-- end badges -->

# Getting started

- This library uses standard C++20 code with support for Windows.
  - We use [`<concepts>`](https://en.cppreference.com/w/cpp/concepts), [`<jthread>`](https://en.cppreference.com/w/cpp/thread/jthread), [`<semaphore>`](https://en.cppreference.com/w/cpp/thread/counting_semaphore) and [`<barrier>`](https://en.cppreference.com/w/cpp/thread/barrier) and for tests we use [`<format>`](https://en.cppreference.com/w/cpp/header/format).
  - Given the [status of C++20](https://github.com/microsoft/STL/wiki/Changelog#vs-2019-1611), the most conformant is VS v16.11.x and this is what has been used to develop and test the code.
- On Windows with VisualStudio, use the Nuget package! 
- Make sure you use `c++latest` as the `<format>` is no longer in the `c++20` option pending ABI resolution.

> **NOTE**
> We are going to track VS 2022 and make full use of C++20 facilities and the API is subject to change.

# API

Utility                   | Description
-------------------------:|:------------
[`siddiqsoft::simple_worker`](#siddiqsoftsimple_worker) | Provides for a single thread with an internal deque.<br/>You'd use this to immediately make any "long" task asynchronous.<br/>Use instead of `std::async`.<br/>Just register your callback/lambda and you're done. No need to worry about waiting for the result (no worry about futures or waiting on them).<br/>Your declared callback will be invoked!
[`siddiqsoft::simple_pool`](#siddiqsoftsimple_pool) | Implements an array of threads backed with a *single* deque. Each thread waits for and processes the next available item from the single deque.
[`siddiqsoft::roundrobin_pool`](#siddiqsoftroundrobin_pool) | Implements an vector of basic_workers (each worker has its independent queue therefore minimizing contention time).<br/>The queue method implements a running counter based round-robin feeder.
[`siddiqsoft::periodic_worker`](#siddiqsoftperiodic_worker) | Provides a facility where you can have your function/lambda invoked at a given periodic rate (in microseconds).

<hr/>

## `siddiqsoft::simple_worker`

Provides for a single thread with an internal deque.

You'd use this to immediately make any "long" task asynchronous where you do not need `std::future` and `std::async`.

Just register your callback/lambda and you're done. No need to worry about waiting for the result (no worry about futures or waiting on them).

```cpp
    template <typename T, uint16_t Pri = 0>
        requires ((Pri >= -10) && (Pri <= 10)) &&
                 std::move_constructible<T>
    struct simple_worker
    {
        simple_worker(simple_worker&) = delete;
        auto operator=(simple_worker&) = delete;

        ~simple_worker();
        simple_worker(simple_worker&& src) noexcept;
        simple_worker(std::function<void(T&)> c) noexcept;

        void queue(T&& item);

        nlohmann::json toJson();

    private:
        std::deque<T>                items {};
        std::shared_mutex            items_mutex {};
        std::counting_semaphore<512> signal {0};
        std::chrono::milliseconds    signalWaitInterval {1500};
        std::function<void(T&)>      callback;
        std::jthread                 processor;
    };
```

<hr/>

## `siddiqsoft::simple_pool`

Implements an array of threads backed with a *single* deque. Each thread waits for and processes the next available item from the single deque.

```cpp
    template <typename T, uint16_t N = 0>
        requires std::move_constructible<T>
    struct simple_pool
    {
        simple_pool(simple_pool&&)            = delete;
        simple_pool& operator=(simple_pool&&) = delete;
        simple_pool(simple_pool&)             = delete;
        simple_pool& operator=(simple_pool&)  = delete;

        ~simple_pool();
        simple_pool(std::function<void(T&)> c);
        
        void queue(T&& item);

        nlohmann::json toJson();

    private:
        std::atomic_uint64_t         queueCounter {0};
        std::vector<std::jthread>    workers {};
        std::function<void(T&)>      callback;
        std::counting_semaphore<512> signal {0};
        std::deque<T>                items {};
        std::shared_mutex            items_mutex;
        std::chrono::milliseconds    signalWaitInterval {1500};
    };
```

<hr/>

## `siddiqsoft::roundrobin_pool`

Implements an vector of basic_workers (each worker has its independent queue therefore minimizing contention time).<br/>The queue method implements a running counter based round-robin feeder.

```cpp
    template <typename T, uint16_t N = 0>
        requires std::move_constructible<T>
    struct roundrobin_pool
    {
        roundrobin_pool(roundrobin_pool&&) = delete;
        auto operator=(roundrobin_pool&&)  = delete;
        roundrobin_pool(roundrobin_pool&)  = delete;
        auto operator=(roundrobin_pool&)   = delete;

        roundrobin_pool(std::function<void(T&)> c);

        void queue(T&& item);

        nlohmann::json toJson();

    private:
        std::atomic_uint64_t          queueCounter {0};
        std::vector<simple_worker<T>> workers {};
        uint64_t                      workersSize {};
        constexpr size_t              nextWorkerIndex()
    };
```

<hr/>

## `siddiqsoft::periodic_worker`

Provides a facility where you can have your function/lambda invoked at a given periodic rate (in microseconds).

You'd instantiate this class with your lambda and period in microseconds.

```cpp
    template <uint16_t Pri = 0>
        requires ((Pri >= -10) && (Pri <= 10))
    struct periodic_worker
    {
        periodic_worker(periodic_worker&)  = delete;
        auto& operator=(periodic_worker&)  = delete;
        periodic_worker(periodic_worker&&) = delete;
        auto& operator=(periodic_worker&&) = delete;

        ~periodic_worker();
        periodic_worker(std::function<void()> c, std::chrono::microseconds) noexcept;

        nlohmann::json toJson();

    private:
        uint64_t                    invokeCounter{0};
        std::chrono::microseconds   invokePeriod {};
        std::counting_semaphore<1>  signal {0};
        std::function<void()>       callback;
        std::jthread                processor;
    };
```

asynchrony : Add asynchrony to your apps
-------------------------------------------
<!-- badges -->
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.asynchrony?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=17&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.asynchrony)
![](https://img.shields.io/github/v/tag/SiddiqSoft/asynchrony)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/17)
<!-- end badges -->

# Motivation
- We needed to add asynchrony to our code.
- The code here is a set of helpers that utilize the underlying deque, semaphore, mutex features found in std.
- Be instructive while providing functional code
- Use only C++20 standard code: jthread, deque, semaphore, barriers and latch

# Usage

> Requires C++20 support!
>
> Specifically we require `jthread` and `stop_token` support which is unfortunately not available in the Apple Clang 16 version. This library works with gcc 14+ or MSVC 17+ or Clang 18+.

Refer to the [documentation](https://siddiqsoft.github.io/asynchrony/) for details.

The library uses concepts to ensure the type `T` meets move construct requirements.

## Single threaded worker

```cpp
#include "siddiqsoft/simple_worker.hpp"

// Define your data
struct MyWork
{
   std::string urlDestination{};
   std::string data{};
   void operator()(){
      magic_post_to(urlDestination, data);
   }
};

void main()
{
   // Declare worker with our data type and the driver function.
   siddiqsoft::simple_worker<MyWork> worker{[](auto& item){
                                              // call the item's operator()
                                              // to invoke actual work.
                                              item();
                                           }};
   // Fire 100 items
   for( int i=0; i < 100; i++ )
   {
      // Queues into the single worker
      worker.queue({std::format("https://localhost:443/test?iter={}",i),
                    "hello-world"});
   }

   // As the user, you must control the lifetime of the worker
   // Trying to delete the worker will cause it to stop
   // and abandon any items in the internal deque.
   std::this_thread::sleep_for(1s);
}

```

## Multi-threaded worker pool

```cpp
#include "siddiqsoft/simple_pool.hpp"

void main()
{
   // Declare worker with our data type and the driver function.
   siddiqsoft::simple_pool<MyWork> worker{[](auto& item){
                                           // call the item's operator()
                                           // to invoke actual work.
                                           item();
                                        }};
   // Fire 100 items
   for( int i=0; i < 100; i++ )
   {
      // Queues into the single queue but multiple worker threads
      // (defaults to CPU thread cout)
      worker.queue({std::format("https://localhost:443/test?iter={}",i),
                    "hello-world"});
   }

   // As the user, you must control the lifetime of the worker
   // Trying to delete the worker will cause it to stop
   // and abandon any items in the internal deque.
   std::this_thread::sleep_for(1s);
}
```


## Multi-threaded roundrobin pool

```cpp
#include "siddiqsoft/roundrobin_pool.hpp"

void main()
{
   // Declare worker with our data type and the driver function.
   siddiqsoft::roundrobin_pool<MyWork> worker{[](auto& item){
                                               // call the item's operator()
                                               // to invoke actual work.
                                               item();
                                             }};
   // Fire 100 items
   for( int i=0; i < 100; i++ )
   {
      // Queues into the thread pools individual queue by round-robin
      // across the threads with simple counter.
      // (defaults to CPU thread cout)
      worker.queue({std::format("https://localhost:443/test?iter={}",i),
                    "hello-world"});
   }

   // As the user, you must control the lifetime of the worker
   // Trying to delete the worker will cause it to stop
   // and abandon any items in the internal deque.
   std::this_thread::sleep_for(1s);
}
```

## Resource Pool

Provides a basic resource pool useful for keeping a pool of connection objects for the various threadpools to checkout/checkin.

```cpp
namespace siddiqsoft {
    template<typename T>
    class resource_pool {
        public:
        auto getCapacity();
        [[nodiscard]] T checkout(); /* throw() */
        void checkin(T&& rsrc);
    };
}
```

## Implementation note
In order to use `std::jthread` on Clang 18 and Clang 19, we enable the compiler flag `"CMAKE_CXX_FLAGS": "-fexperimental-library"` in the CMakeLists.txt. This option will show up in your client library under Clang compilers.

<p align="right">
&copy; 2021 Siddiq Software LLC. All rights reserved.
</p>

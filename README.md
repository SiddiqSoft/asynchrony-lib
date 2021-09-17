asynchrony : Add asynchrony to your apps
-------------------------------------------
<!-- badges -->
[![CodeQL](https://github.com/SiddiqSoft/asynchrony-lib/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/SiddiqSoft/asynchrony-lib/actions/workflows/codeql-analysis.yml)
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.asynchrony-lib?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=17&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.asynchrony-lib)
![](https://img.shields.io/github/v/tag/SiddiqSoft/asynchrony-lib)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/17)
![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/17)
<!-- end badges -->

# Motivation
- We needed to add asynchrony to our code. The minimal effort seemed reasonable "packagable" into a header file.
- Use only C++20 standard code: jthread, deque, semaphore, barriers and latch
- No external dependency

# Usage

Refer to the [documentation](https://siddiqsoft.github.io/asynchrony-lib/) for details.

The library uses concepts to ensure the type `T` meets copy and move construct requirements.

## Single threaded worker

```cpp
#include "siddiqsoft/asynchrony-lib.hpp"

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
   siddiqsoft::basic_worker<MyWork> worker{[](auto& item){
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
#include "siddiqsoft/basic_pool.hpp"

void main()
{
   // Declare worker with our data type and the driver function.
   siddiqsoft::basic_pool<MyWork> worker{[](auto& item){
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

<p align="right">
&copy; 2021 Siddiq Software LLC. All rights reserved.
</p>

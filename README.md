# weejobs
Weejobs is a simple header-only C++11 API for scheduling jobs to run in the background. It consists of a single header file and has no dependencies aside from the STL.

## Setup

Use this macro somewhere in your app, in a source file (not a header).
```c++
WEEJOBS_INSTANCE;
```

The default namespace for weejobs is `jobs`. You can customize that by setting a macro before including the header:
```c++
#define WEEJOBS_NAMESPACE my_jobs_namespace
#include <weejobs.h>
```

If you plan to use the weejobs API from a shared module (DLL) under Windows, you will also need to set the export directive:
```c++
#define WEEJOBS_EXPORT MY_PROJECT_EXPORT
```

## Examples - Running Jobs

### Dispatching a job
The simplest usage is to spawn a job with no return value (fire and forget):
```c++
    auto job = []() { std::cout << "Hello, world!" << std::endl; };
    jobs::dispatch(job);
```

### Getting a future result
You can also spawn a job and get a "future result." In this case, the job's signature should take a `jobs::cancelable&` argument so it can check for cancelation:
```c++
    auto job = [](jobs::cancelable&) { return 7; };
    jobs::future<int> result = jobs::dispatch(job);
    
    // later...
    if (result.available())
       std::cout << "Result = " << result.value() << std::endl;
    else if (result.canceled())
       std::cout << "Job was canceled" << std::endl;
    else
       // still running.... come back later?
```

### Waiting for a job to finish
Use `join()` to block until a job completes:
```c++
    auto job = [url](jobs::cancelable&) {
        return fetch_data_from_network(url);
    };
    auto result = jobs::dispatch(job);
    ...
    auto value = result.join();
```

### Chaining jobs together
To automatically start a new job when another job completes, you can use the `then` construct. In the example below, the result of `task1` (an integer) gets passed to `task2` as soon as the result of `task1` becomes available:
```c++
    auto task1 = [](jobs::cancelable& c)
    {
       return 7;
    };
    
    auto task2 = [](const int& input, jobs::cancelable& c)
    {
        return input * 2;
    };
    
    auto result1 = jobs::dispatch(task1);
    auto result2 = result1.then_dispatch<int>(task2);
```

### Checking for cancelation
Here's how to check for cancelation inside a job. If you dispatch a job, and the `future` goes out of scope, `cancelable.canceled()` will return true and the job can exit early instead of wasting resources.
```c++
   auto job = [url](jobs::cancelable& state) {
       std::string data;
       if (!state.canceled())
           data = fetch_data_from_network(url);
       return data;
   };

   auto result = jobs::dispatch(job);
   
   // if "result" goes out of scope, `state.canceled()` in the job will return true
   // AND `result.canceled()` will be true.
```

## Examples - Controlling Jobs

### Job pools
You can dispatch a job to a specific job pool. A job pool is just a collection of dedicated threads with a priority queue. Use `jobs::get_pool` to get or create a job pool. You can use `set_concurrency` to allocate the number of threads the pool should use (the default is 2).
```c++
    auto my_pool = jobs::get_pool("My Job Pool");
    my_pool.set_concurrency(4);
    
    auto job = []() { std::cout << "Hello, world!" << std::endl; };

    jobs::context context;
    context.pool = my_pool;

    jobs::dispatch(job, context);
```

### Prioritizing jobs
Each job pool has a priority queue. You can use a lambda function in your `context` to specify the priority of a job. Since it uses a lambda function, the priorty can change between the time you dispatch the job and the time it gets pulled from the queue and executed:
```c++
    jobs::context context;
    context.name = "My Job";
    context.priority = []() { return calculate_priority(); };
    
    jobs::dispatch(job, context);
```

### Grouping jobs
You can group jobs together. This lets you dispatch a number of jobs and then wait for all of them to finish before continuing:
```c++
    jobs::context context;
    context.group = jobs::jobgroup::create();
    
    jobs::dispatch(task1, context);
    jobs::dispatch(task2, context);
    jobs::dispatch(task3, context);

    // block until all 3 jobs are finished
    group.join();
```    

## Background

This SDK exists because existing solutions did not support two things we needed: automatic job cancelation, and job prioritization.

## License

Weejobs is distributed under the MIT license.

Weejobs is Copyright 2024+ Pelican Mapping.

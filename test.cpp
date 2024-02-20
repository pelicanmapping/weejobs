#include <chrono>
#include <iostream>
#include <string>
#include "weejobs.h"

static std::mutex print_lock;

WEEJOBS_INSTANCE;
using namespace std::chrono_literals;

void atomic_print(const std::string& str)
{
    std::lock_guard<std::mutex> L(print_lock);
    std::cout << str << std::endl;
}

int main(int argc, char** argv)
{
    jobs::get_pool()->set_concurrency(4);

    // fire and forget - no args, no return value
    auto fire_and_forget = []()
        {
            atomic_print("Running fire and forget job");
        };
    jobs::dispatch(fire_and_forget);

    // future result - return value, cancelable& required
    auto get_future_result = [](jobs::cancelable& c)
        {
            atomic_print("Running future result job");
            return 42;
        };
    auto future_result = jobs::dispatch(get_future_result);

    // Join
    auto result = future_result.join();
    atomic_print("Future result = " + std::to_string(result));

    // Chaining
    auto chain_job1 = [](jobs::cancelable& c)
        {
            atomic_print("Running chain job 1");
            return 42;
        };
    auto chain_job2 = [](const int& i, jobs::cancelable& c)
        {
            atomic_print("Running chain job 2");
            return i * 2;
        };
    auto chain_job3 = [](const int& i)
        {
            int result = i * 2;
            atomic_print("Running chain job 3 (fire and forget), result = " + std::to_string(result));
        };

    auto chain1 = jobs::dispatch(chain_job1);
    auto chain2 = chain1.then_dispatch(chain_job2);
    chain2.then_dispatch(chain_job3);
    atomic_print("Chain result = " + std::to_string(chain2.join()));

    // Cancelation
    auto cancelable_task = [](jobs::cancelable& c)
        {
            if (c.canceled())
                atomic_print("CANCELED Cancelable task");
            else
                atomic_print("Running cancelable task");
            return 42;
        };
    auto cancelable_result = jobs::dispatch(cancelable_task);
    if (cancelable_result.canceled())
        atomic_print("Canceleable result = CANCELED");
    else
        atomic_print("Canceleable Result = " + std::to_string(cancelable_result.join()));

    // User promise
    auto user_promise_job = [](jobs::promise<int>& promise)
        {
            promise.resolve(66);
        };
    jobs::promise<int> my_promise;
    auto user_promise_result = jobs::dispatch(user_promise_job, my_promise);
    atomic_print("User promise result = " + std::to_string(user_promise_result.join()));

    // Job pools
    jobs::context context;

    context.pool = jobs::get_pool("custom pool");
    jobs::dispatch([]() {
            atomic_print("Running in custom pool");
        }, context);

    // Priority
    context.priority = []() { return 10.0f; };
    jobs::dispatch([]() {
            atomic_print("Running with priority 10");
        }, context);

    // Auto-cancelation off
    context.can_cancel = false;
    auto non_cancelable_task = [](jobs::cancelable& c)
        {
            if (c.canceled())
                atomic_print("CANCELED Non-cancelable task");
            else
                atomic_print("Running non-cancelable task");
            return 13;
        };
    auto non_cancelable_result = jobs::dispatch(non_cancelable_task, context);

    // Grouping
    context.group = jobs::jobgroup::create();

    jobs::dispatch([]() {
            atomic_print("Running group job 1");
        }, context);

    jobs::dispatch([]() {
            atomic_print("Running group job 2");
        }, context);

    jobs::dispatch([]() {
            atomic_print("Running group job 3");
        }, context);

    context.group->join(); // wait for all 3 to finish
    atomic_print("All group jobs finished");

    // At exit, queued jobs will be discarded and running jobs will be joined.
    return 0;
}

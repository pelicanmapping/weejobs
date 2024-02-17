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
    jobs::context con;
    con.group = jobs::jobgroup::create();
    con.can_cancel = false;

    jobs::get_pool()->set_concurrency(4);

    auto second_task = [](int i, jobs::cancelable& c)
        {
            if (c.canceled())
                atomic_print("CANCELED Second task " + std::to_string(i));
            else
                atomic_print("Second task " + std::to_string(i));
            return i;
        };

    auto third_task = [](auto i, jobs::cancelable& c)
        {
            if (c.canceled())
                atomic_print("CANCELED Third task " + std::to_string(i));
            else
                atomic_print("Third task " + std::to_string(i));
            return i;
        };

    auto fourth_task = [](auto i, jobs::promise<int> promise)
        {
            atomic_print("Fourth task " + std::to_string(i));
            promise.resolve(i);
        };

    auto fifth_task = [](auto i)
        {
            atomic_print("Fifth task " + std::to_string(i));
        };
    
    for(int i=0; i<8; ++i)
    {
        auto task = [i](jobs::cancelable& c)
        {
            atomic_print("Start task " + std::to_string(i));
            std::this_thread::sleep_for(250ms);
            atomic_print("End task " + std::to_string(i));
            return i;
        };
        
        jobs::dispatch(task, con)
            .send_result_to(second_task, con)
            .send_result_to(third_task, con)
            .send_result_to<int>(fourth_task, con)
            .send_result_to<int>(fifth_task, con);         
            
    }

    con.group->join();

    return 0;
}


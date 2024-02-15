#include "weejobs.h"
#include <chrono>
#include <iostream>
#include <string>

static std::mutex print_lock;

WEEJOBS_INSTANCE;
using namespace std::chrono_literals;

void atomic_print(const std::string& str)
{
    std::unique_lock<std::mutex> L(print_lock);
    std::cout << str << std::endl;
}

int main(int argc, char** argv)
{    
    jobs::context c;
    c.group = jobs::jobgroup::create();

    jobs::get_pool()->set_concurrency(4);
    
    for(int i=0; i<8; ++i)
    {
        auto task = [i]()
        {
            atomic_print("Start task " + std::to_string(i));
            std::this_thread::sleep_for(1s);
            atomic_print("End task " + std::to_string(i));
        };
        
        jobs::dispatch(task, c);
    }
    
    //c.group->join();
    std::this_thread::sleep_for(1s);
    return 0;
}


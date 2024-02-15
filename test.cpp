#include "weejobs.h"
#include <chrono>
#include <iostream>

WEEJOBS_INSTANCE;
using namespace std::chrono_literals;

int main(int argc, char** argv)
{    
    jobs::context c;
    c.group = jobs::jobgroup::create();
    
    for(int i=0; i<8; ++i)
    {
        auto task = [i]()
        {
            std::cout << "Start task " << i << std::endl;
            std::this_thread::sleep_for(1s);
            std::cout << "End task " << i << std::endl;
        };
        
        jobs::dispatch(task, c);
    }
    
    //c.group->join();
    std::this_thread::sleep_for(1s);
    return 0;
}


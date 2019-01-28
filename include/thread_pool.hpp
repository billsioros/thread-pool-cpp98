
#pragma once

#include <cstddef>

namespace thread_pool
{
    void create(std::size_t size);
    
    void destroy();

    void block();

    void schedule(void (*task)(void *), void * args);
}

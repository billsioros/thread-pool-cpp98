
#include <thread_pool.hpp>
#include <iostream>

static const char * messages[] =
{
    "Hello",
    "darkness",
    "my",
    "old",
    "friend"
};

void print(void * arg)
{
    char * message = static_cast<char *>(arg);

    std::cout << message << std::endl;
}

int main()
{
    thread_pool::create(10UL);

    for (std::size_t i = 0UL; i < (sizeof(messages) / sizeof(char *)); i++)
    {
        thread_pool::schedule(print, (void *)messages[i]);
    }

    thread_pool::block();

    thread_pool::destroy();

    return 0;
}

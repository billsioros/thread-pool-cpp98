
#include <thread_pool.hpp>
#include <list>

#include <iostream>     // std::cerr
#include <cstring>      // strerror_r
#include <cassert>      // assert
#include <pthread.h>    // pthread_t

static struct Scheduler
{
    pthread_mutex_t mtx;

    bool running;

    pthread_t * threads; std::size_t size;
    
    struct Job
    {
        static std::size_t ongoing;

        static pthread_cond_t all_finished_cnd;

        void (*task)(void *);
        
        void * args;

        Job(void (*task)(void *), void * args) : task(task), args(args) {}
    };

    struct Queue : public std::list<Job>
    {
        pthread_cond_t non_empty_cnd;

        Queue() : non_empty_cnd(PTHREAD_COND_INITIALIZER) {};

    } queue;

    Scheduler()
    :
    mtx(PTHREAD_MUTEX_INITIALIZER), running(false), threads(nullptr), size(0UL)
    {
    }

} scheduler;

std::size_t Scheduler::Job::ongoing = 0UL;
        
pthread_cond_t Scheduler::Job::all_finished_cnd = PTHREAD_COND_INITIALIZER;

enum exception
{
    lock_acquire, lock_release,
    cond_signal, cond_broadcast, cond_wait,
    thrd_create, thrd_join
};

static const char * messages[] =
{
    [exception::lock_acquire]   = "An exception occurred in the midst of acquiring the job-queue's mutex",
    [exception::lock_release]   = "An exception occurred in the midst of releasing the job-queue's mutex",
    [exception::cond_signal]    = "An exception occurred in the midst of signaling the job-queue's condition variable",
    [exception::cond_broadcast] = "An exception occurred in the midst of broadcasting the job-queue's condition variable",
    [exception::cond_wait]      = "An exception occurred in the midst of waiting on the job-queue's condition variable",
    [exception::thrd_create]    = "An exception occurred in the midst of instantiating a new thread",
    [exception::thrd_join]      = "An exception occurred in the midst of joining a thread"
};

#define panic(error, errno)                                     \
do                                                              \
{                                                               \
    char buff[256UL];                                           \
                                                                \
    std::cerr                                                   \
    << __FILE__ << ":" << __func__ << ":" << __LINE__ << ":~ "  \
    << messages[error]                                          \
    << " [" << strerror_r(errno, buff, sizeof(buff)) << "]"     \
    << std::endl;                                               \
                                                                \
    std::exit(EXIT_FAILURE);                                    \
} while (false)                                                 \

static void * idle(void *)
{
    for (;;)
    {
        int code;
        if ((code = pthread_mutex_lock(&scheduler.mtx)))
            panic(exception::lock_acquire, code);

        while (scheduler.running && scheduler.queue.empty())
            if ((code = pthread_cond_wait(&scheduler.queue.non_empty_cnd, &scheduler.mtx)))
                panic(exception::cond_wait, code);

        if (scheduler.running)
        {
            Scheduler::Job job = scheduler.queue.front(); scheduler.queue.pop_front();

            Scheduler::Job::ongoing++;

            if ((code = pthread_mutex_unlock(&scheduler.mtx)))
                panic(exception::lock_release, code);
            
            job.task(job.args);
            
            if ((code = pthread_mutex_lock(&scheduler.mtx)))
                panic(exception::lock_acquire, code);

            if (!--Scheduler::Job::ongoing && scheduler.queue.empty())
                if ((code = pthread_cond_signal(&Scheduler::Job::all_finished_cnd)))
                    panic(exception::cond_signal, code);

            if ((code = pthread_mutex_unlock(&scheduler.mtx)))
                panic(exception::lock_release, code);
        }
        else
        {
            if ((code = pthread_mutex_unlock(&scheduler.mtx)))
                panic(exception::lock_release, code);

            break;
        }
    }

    pthread_exit(nullptr);
}

void thread_pool::create(std::size_t size)
{
    int code;
    if ((code = pthread_mutex_lock(&scheduler.mtx)))
        panic(exception::lock_acquire, code);

    assert(!scheduler.running && !Scheduler::Job::ongoing);

    if ((code = pthread_mutex_unlock(&scheduler.mtx)))
        panic(exception::lock_release, code);

    scheduler.running = true;

    scheduler.threads = new pthread_t[scheduler.size = size];

    for (std::size_t i = 0UL; i < size; i++)
        if ((code = pthread_create(&scheduler.threads[i], nullptr, idle, nullptr)))
            panic(exception::thrd_create, code);
}

void thread_pool::destroy()
{
    int code;
    if ((code = pthread_mutex_lock(&scheduler.mtx)))
        panic(exception::lock_acquire, code);

    assert(scheduler.running);

    scheduler.running = false;

    scheduler.queue.clear();

    if ((code = pthread_cond_broadcast(&scheduler.queue.non_empty_cnd)))
        panic(exception::cond_broadcast, code);

    if ((code = pthread_mutex_unlock(&scheduler.mtx)))
        panic(exception::lock_release, code);

    for (std::size_t i = 0; i < scheduler.size; i++)
        if ((code = pthread_join(scheduler.threads[i], nullptr)))
            panic(exception::thrd_join, code);

    delete[] scheduler.threads;
    
    scheduler.threads = nullptr; scheduler.size = 0UL;
}

void thread_pool::block()
{
    int code;
    if ((code = pthread_mutex_lock(&scheduler.mtx)))
        panic(exception::lock_acquire, code);

    assert(scheduler.running);

    while (scheduler.queue.size() || Scheduler::Job::ongoing)
        if ((code = pthread_cond_wait(&Scheduler::Job::all_finished_cnd, &scheduler.mtx)))
            panic(exception::cond_wait, code);

    if ((code = pthread_mutex_unlock(&scheduler.mtx)))
        panic(exception::lock_release, code);
}

void thread_pool::schedule(void (*task)(void *), void * args)
{
    int code;
    if ((code = pthread_mutex_lock(&scheduler.mtx)))
        panic(exception::lock_acquire, code);

    assert(scheduler.running);

    scheduler.queue.emplace_back(task, args);

    if ((code = pthread_cond_signal(&scheduler.queue.non_empty_cnd)))
        panic(exception::cond_signal, code);

    if ((code = pthread_mutex_unlock(&scheduler.mtx)))
        panic(exception::lock_release, code);
}

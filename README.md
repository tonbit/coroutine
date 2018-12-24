# C++11 single .h asymmetric coroutine implementation

### API

in namespace coroutine:        
* routine_t create(std::function<void()> f);
* void destroy(routine_t id);
* int resume(routine_t id);
* void yield();
* TYPE await(TYPE(*f)());
* routine_t current();
* class Channel<T> with push()/pop();

### OS

* Linux
* macOS
* Windows

### Demo
						
```cpp
#include <iostream>
#include "coroutine.h"
#include <chrono>
#include <sstream>

coroutine::Channel<int> channel;

#define LogFN(x) \
{ \
    std::ostringstream os; \
    os << __FUNCTION__ << " " << x; \
    std::cout << os.str() << std::endl; \
}

string async_func()
{
    LogFN("sleep_for 3000")
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    LogFN("end")
    return "end";
}

void routine_func1()
{
    LogFN("channel.pop()");
    int i = channel.pop();
    LogFN(i);

    LogFN("channel.pop()");
    i = channel.pop();
    LogFN(i);
}

void routine_func2(int i)
{
    LogFN("coroutine::yield()");
    coroutine::yield();
    LogFN("coroutine::yield() end");

    LogFN("await(async_func)");

    //run function async
    //yield current routine if result not returned
    string str = coroutine::await(async_func);
    LogFN("await(async_func)" << str);
}

void thread_func()
{
    //create routine with callback like std::function<void()>
    coroutine::routine_t rt1 = coroutine::create(routine_func1);
    LogFN("create rt1");

    coroutine::routine_t rt2 = coroutine::create(std::bind(routine_func2, 2));
    LogFN("create rt2");

    coroutine::routine_t rt3 = coroutine::create(routine_func1);
    LogFN("create rt3");

    LogFN("resume rt1");
    coroutine::resume(rt1);

    LogFN("resume rt2");
    coroutine::resume(rt2);

    LogFN("resume rt3");
    coroutine::resume(rt3);

    LogFN("channel.push(10)");
    channel.push(10);

    LogFN("resume rt2");
    coroutine::resume(rt2);

    LogFN("channel.push(11)");
    channel.push(11);

    LogFN("sleep for 6000");

    std::this_thread::sleep_for(std::chrono::milliseconds(6000));

    LogFN("resume rt2");
    coroutine::resume(rt2);

    //destroy routine, free resource allocated
    //Warning: don't destroy routine by itself
    LogFN("destroy rt1");
    coroutine::destroy(rt1);

    LogFN("destroy rt2");
    coroutine::destroy(rt2);

    LogFN("destroy rt3");
    coroutine::destroy(rt3);
}

int main()
{
    std::thread t1(thread_func);
    std::thread t2([]() {
        //unsupported coordinating routine crossing threads
    });
    t1.join();
    t2.join();

    return 0;
}
```

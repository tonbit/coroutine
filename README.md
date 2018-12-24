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
// coroutineTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "coroutine.h"
#include <chrono>
#include <sstream>

coroutine::Channel<int> channel;

#define LogF(x) \
{ \
    std::ostringstream os; \
    time_t now = time(0); \
    os << __FUNCTION__ << " " << x; \
    std::cout << os.str() << std::endl; \
}

string async_func()
{
    LogF("sleep_for 3000")
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    LogF("end")
    return "end";
}

void routine_func1()
{
    LogF("channel.pop()");
    int i = channel.pop();
    LogF(i);

    LogF("channel.pop()");
    i = channel.pop();
    LogF(i);
}

void routine_func2(int i)
{
    LogF("coroutine::yield()");
    coroutine::yield();
    LogF("coroutine::yield() end");

    LogF("await(async_func)");

    //run function async
    //yield current routine if result not returned
    string str = coroutine::await(async_func);
    LogF("await(async_func)" << str);
}

void thread_func()
{
    //create routine with callback like std::function<void()>
    coroutine::routine_t rt1 = coroutine::create(routine_func1);
    LogF("create rt1");

    coroutine::routine_t rt2 = coroutine::create(std::bind(routine_func2, 2));
    LogF("create rt2");

    coroutine::routine_t rt3 = coroutine::create(routine_func1);
    LogF("create rt3");

    LogF("resume rt1");
    coroutine::resume(rt1);

    LogF("resume rt2");
    coroutine::resume(rt2);

    LogF("resume rt3");
    coroutine::resume(rt3);

    LogF("channel.push(10)");
    channel.push(10);

    LogF("resume rt2");
    coroutine::resume(rt2);

    LogF("channel.push(11)");
    channel.push(11);

    LogF("sleep for 6000");

    std::this_thread::sleep_for(std::chrono::milliseconds(6000));

    LogF("resume rt2");
    coroutine::resume(rt2);

    //destroy routine, free resource allocated
    //Warning: don't destroy routine by itself
    LogF("destroy rt1");
    coroutine::destroy(rt1);

    LogF("destroy rt2");
    coroutine::destroy(rt2);

    LogF("destroy rt3");
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

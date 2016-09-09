# C++11 single header file asymmetric coroutine implementation

```cpp
#include "coroutine.h"
using namespace coroutine;

string async_func()
{
	return "...";
}

void routine_func1()
{
	printf("1 > ");
	
	//yield current routine, swap to resume point
	yield();

	//await result of function async called 
	//if await result timeout in routine, yield()
	string str = await([]() {return "1"; });

	printf("2 > ");
}

void routine_func2(int i)
{
	printf("3 > ");
	yield();
}

void thread_func()
{
	//create routine with callback like std::function<void()>
	routine_t rt1 = create_routine(routine_func1);
	routine_t rt2 = create_routine(std::bind(routine_func2, 2));
	int rc;
	
	while (true)
	{
		//if resume routine new created, its callback will start running
		rc = resume(rt1);
		
		//if resume routine suspended, swap to its yield point
		rc = resume(rt1);
		
		//if routine has destroyed, resume it will return -1
		//if routine has exited, resume it will return -2

		rc = resume(rt2);
		if (rc1 != 0)
			break;
	}

	//destroy routine, free resouce
	//don't destroy routine by itself
	destroy_routine(rt1);
	destroy_routine(rt2);
}

int main()
{
	std::thread t1(thread_func);
	std::thread t2([](){
		//not support to coordinate routine cross threads
	});
	t1.join();
	t2.join();
	return 0;
}
```
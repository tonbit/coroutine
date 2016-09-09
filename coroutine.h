/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef STDEX_COROUTINE_H_
#define STDEX_COROUTINE_H_

#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <string>
using std::string;
using std::wstring;

#include <array>
#include <vector>
#include <list>
#include <map>

#include <utility>
#include <thread>
#include <future>

#ifdef _MSC_VER

#include <Windows.h>

namespace coroutine {

typedef unsigned routine_t;

struct Routine
{
	std::function<void()> func;
	bool finished;
	LPVOID fiber;

	Routine(std::function<void()> f)
	{
		func = f;
		finished = false;
		fiber = nullptr;
	}

	~Routine()
	{
		DeleteFiber(fiber);
	}
};

struct Ordinator
{
	std::vector<Routine *> routines;
	routine_t current;
	LPVOID fiber;

	Ordinator()
	{
		fiber = ConvertThreadToFiber(nullptr);
	}

	~Ordinator()
	{
		for (auto &routine : routines)
			delete routine;
		routines.clear();
	}
};

thread_local static Ordinator ordinator;

inline routine_t create_routine(std::function<void()> f)
{
	Routine *routine = new Routine(f);
	ordinator.routines.push_back(routine);
	return ordinator.routines.size();
}

inline void destroy_routine(routine_t id)
{
	Routine *routine = ordinator.routines[id-1];
	assert(routine != nullptr);
	
	delete routine;
	ordinator.routines[id-1] = nullptr;
}

inline void __stdcall entry(LPVOID lpParameter)
{
	routine_t id = ordinator.current;
	Routine *routine = ordinator.routines[id-1];
	assert(routine != nullptr);

	routine->func();

	routine->finished = true;
	ordinator.current = 0;
	
	SwitchToFiber(ordinator.fiber);
}

inline int resume(routine_t id)
{
	assert(ordinator.current == 0);

	Routine *routine = ordinator.routines[id-1];
	if (routine == nullptr)
		return -1;

	if (routine->finished)
		return -2;

	if (routine->fiber == nullptr)
	{
		routine->fiber = CreateFiber(0, entry, nullptr);
		ordinator.current = id;
		SwitchToFiber(routine->fiber);
	}
	else
	{
		ordinator.current = id;
		SwitchToFiber(routine->fiber);
	}

	return 0;
}

inline void yield()
{
	routine_t id = ordinator.current;
	Routine *routine = ordinator.routines[id-1];
	assert(routine != nullptr);

	ordinator.current = 0;
	SwitchToFiber(ordinator.fiber);
}

routine_t current_routine()
{
	return ordinator.current;
}

#if 0
template<class Function>
typename std::result_of<Function()>::type
await(Function&& func)
{
	auto future = std::async(std::launch::async, func);
	std::future_status status = future.wait_for(std::chrono::milliseconds(10));

	while (status == std::future_status::timeout)
	{
		if (ordinator.current != 0)
			yield();

		status = future.wait_for(std::chrono::milliseconds(10));
	}
	return future.get();
}
#endif

#if 1
template<class Function>
std::result_of_t<std::decay_t<Function>()>
await(Function&& func)
{
	auto future = std::async(std::launch::async, func);
	std::future_status status = future.wait_for(std::chrono::milliseconds(10));

	while (status == std::future_status::timeout)
	{
		if (ordinator.current != 0)
			yield();

		status = future.wait_for(std::chrono::milliseconds(10));
	}
	return future.get();
}
#endif

}

#else

#include <ucontext.h>

namespace coroutine {

typedef unsigned routine_t;

struct Routine
{
	std::function<void()> func;
	char *stack;
	ptrdiff_t stack_size;
	ptrdiff_t capacity;
	bool finished;
	ucontext_t ctx;

	Routine(std::function<void()> f)
	{
		func = f;
		stack = nullptr;
		stack_size = 0;
		capacity = 0;
		finished = false;
	}

	~Routine()
	{
		if (stack)
		{
			free(stack);
		}
	}
};

struct Ordinator
{
	std::vector<Routine *> routines;
	routine_t current;
	char *stack;
	size_t stack_size;
	ucontext_t ctx;

	inline Ordinator(size_t ss = 1024*1024)
	{
		current = 0;
		stack = (char *)malloc(ss);
		stack_size = ss;
	}

	inline ~Ordinator()
	{
		for (auto &routine : routines)
		{
			if (routine)
				delete routine;
		}
		free(stack);
	}
};

thread_local static Ordinator ordinator;

inline routine_t create_routine(std::function<void()> f)
{
	Routine *routine = new Routine(f);
	ordinator.routines.push_back(routine);
	return ordinator.routines.size();
}

inline void destroy_routine(routine_t id)
{
	Routine *routine = ordinator.routines[id-1];
	assert(routine != nullptr);
	
	delete routine;
	ordinator.routines[id-1] = nullptr;
}

inline void entry()
{
	routine_t id = ordinator.current;
	Routine *routine = ordinator.routines[id-1];
	routine->func();

	routine->finished = true;
	ordinator.current = 0;
}

inline int resume(routine_t id)
{
	assert(ordinator.current == 0);

	Routine *routine = ordinator.routines[id-1];
	if (routine == nullptr)
		return -1;

	if (routine->finished)
		return -2;
	
	if (routine->stack == nullptr)
	{
		//initializes the structure to the currently active context.
		//When successful, getcontext() returns 0
		//On error, return -1 and set errno appropriately.
		getcontext(&routine->ctx);

		//Before invoking makecontext(), the caller must allocate a new stack
		//for this context and assign its address to ucp->uc_stack,
		//and define a successor context and assign its address to ucp->uc_link.
		routine->ctx.uc_stack.ss_sp = ordinator.stack;
		routine->ctx.uc_stack.ss_size = ordinator.stack_size;
		routine->ctx.uc_link = &ordinator.ctx;
		ordinator.current = id;

		//When this context is later activated by swapcontext(), the function entry is called.
		//When this function returns, the  successor context is activated.
		//If the successor context pointer is NULL, the thread exits.
		makecontext(&routine->ctx, (void (*)(void))entry, 0);

		//The swapcontext() function saves the current context,
		//and then activates the context of another.
		swapcontext(&ordinator.ctx, &routine->ctx);
	}
	else
	{
		memcpy(ordinator.stack + ordinator.stack_size - routine->stack_size, routine->stack, routine->stack_size);
		ordinator.current = id;
		swapcontext(&ordinator.ctx, &routine->ctx);
	}

	return 0;
}

inline void yield()
{
	routine_t id = ordinator.current;
	Routine *routine = ordinator.routines[id-1];

	char *stack_top = ordinator.stack + ordinator.stack_size;
	char stack_bottom = 0;

	assert(stack_top - &stack_bottom <= ordinator.stack_size);

	if (routine->capacity < stack_top - &stack_bottom)
	{
		free(routine->stack);
		routine->capacity = stack_top - &stack_bottom;
		routine->stack = (char *)malloc(routine->capacity);
	}
	routine->stack_size = stack_top - &stack_bottom;
	memcpy(routine->stack, &stack_bottom, routine->stack_size);

	ordinator.current = 0;
	swapcontext(&routine->ctx , &ordinator.ctx);
}

routine_t current_routine()
{
	return ordinator.current;
}

template<class Function>
typename std::result_of<Function()>::type
await(Function&& func)
{
	auto future = std::async(std::launch::async, func);
	std::future_status status = future.wait_for(std::chrono::milliseconds(10));

	while (status == std::future_status::timeout)
	{
		if (ordinator.current != 0)
			yield();

		status = future.wait_for(std::chrono::milliseconds(10));
	}
	return future.get();
}

}
#endif
#endif //STDEX_COROUTINE_H_

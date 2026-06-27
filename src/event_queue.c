/*
	event_queue.c ~ RL
*/

#include "event_queue.h"
#include <Windows.h>

#define IS_RUNNING_FROM_WINDOW() (GetCurrentThreadId() != main_thread_id)
#define IS_RUNNING_FROM_CONSOLE() (GetCurrentThreadId() == main_thread_id)

struct queued_event
{
	handle_generic_t handler;
	const void* data;
};

/* since the queue is only for window->main cause main->window is handled by the win32 message queue, no mutex is needed */
static volatile LONG queue_lock;

static struct queued_event queue[16];
static int top;

static uint8_t main_arena[0x400];
static uint8_t window_arena[0x400];
static volatile LONG main_arena_top;
static volatile LONG window_arena_top;

static volatile LONG pending_main_arena_events;

static DWORD main_thread_id;
static HWND window;

void queue_set_main_thread_id(DWORD id)
{
	main_thread_id = id;
}

void queue_set_window_handle(HWND wnd)
{
	window = wnd;
}

void* queue_copy_data(const void* data, size_t size)
{
	if (IS_RUNNING_FROM_CONSOLE())
	{
		InterlockedIncrement(&pending_main_arena_events);
	}

	uint8_t* arena = IS_RUNNING_FROM_CONSOLE() ? main_arena : window_arena;
	LONG arena_top = IS_RUNNING_FROM_CONSOLE() ? main_arena_top : window_arena_top;

	RUNTIME_ASSERT(arena_top + size < sizeof main_arena);
	memcpy(arena + arena_top, data, size);
	InterlockedAdd(IS_RUNNING_FROM_CONSOLE() ? &main_arena_top : &window_arena_top, (LONG)size);
	return arena + arena_top;
}

static inline void queue_lock_acquire(volatile LONG* lock)
{
	while (InterlockedCompareExchange(lock, 1, 0) != 0)
	{
		Sleep(0);
	}
}

static inline void queue_lock_release(volatile LONG* lock)
{
	InterlockedExchange(lock, 0);
}

void queue_add_from_main(handle_generic_t handler, const void* data)
{
	if (IS_RUNNING_FROM_CONSOLE())
	{
		handler(data);
		return;
	}
	queue_add(handler, data);
}

void queue_add_from_window(handle_generic_t handler, const void* data)
{
	if (IS_RUNNING_FROM_WINDOW())
	{
		handler(data);
		return;
	}
	queue_add(handler, data);
}

void queue_add(handle_generic_t handler, const void* data)
{
	if (!handler)
	{
		return;
	}
	if (IS_RUNNING_FROM_CONSOLE())
	{
		debug_format("Adding event to window\n");
		PostMessageW(window, QUEUE_MSG_MAIN_TO_WINDOW, (WPARAM)handler, (LPARAM)data);
		return;
	}
	debug_format("Adding event to main\n");
	queue_lock_acquire(&queue_lock);
	queue[top++] = (struct queued_event){ .handler = handler, .data = data };
	queue_lock_release(&queue_lock);
}

int queue_run(void)
{
	if (IS_RUNNING_FROM_WINDOW())
	{
		return 0;
	}

	queue_lock_acquire(&queue_lock);
	int res = top;
	while (top > 0)
	{
		struct queued_event event = queue[--top];
		event.handler(event.data);
	}
	queue_lock_release(&queue_lock);
	InterlockedExchange(&window_arena_top, 0);
	return res;
}

void queue_pop_window_event(void)
{
	if (IS_RUNNING_FROM_CONSOLE())
	{
		return;
	}

	InterlockedDecrement(&pending_main_arena_events);
	if (InterlockedCompareExchange(&pending_main_arena_events, 0, 0) <= 0)
	{
		InterlockedExchange(&main_arena_top, 0);
	}
}
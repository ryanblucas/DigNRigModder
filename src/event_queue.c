/*
	event_queue.c ~ RL
*/

#include "event_queue.h"
#include <Windows.h>

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
static size_t main_arena_top;
static uint8_t window_arena[0x400];
static size_t window_arena_top;

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
	uint8_t* arena = GetCurrentThreadId() == main_thread_id ? main_arena : window_arena;
	size_t* arena_top = GetCurrentThreadId() == main_thread_id ? &main_arena_top : &window_arena_top;

	RUNTIME_ASSERT(*arena_top + size < sizeof main_arena);
	memcpy(arena + *arena_top, data, size);
	*arena_top += size;
	return arena + *arena_top - size;
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

void queue_add(handle_generic_t handler, const void* data)
{
	if (!handler)
	{
		return;
	}
	if (GetCurrentThreadId() == main_thread_id)
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
	if (GetCurrentThreadId() != main_thread_id)
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
	main_arena_top = 0;
	return res;
}
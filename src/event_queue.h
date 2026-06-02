/*
	event_queue.h ~ RL
	For enqueuing events from thread to thread
*/

#pragma once

#include "types.h"
#include <Windows.h>

/* WPARAM is handler proc, LPARAM is data */
#define QUEUE_MSG_MAIN_TO_WINDOW (WM_USER + 0x80)

typedef void (*handle_generic_t)(const void* data);

void queue_set_main_thread_id(DWORD id);
void queue_set_window_handle(HWND wnd);

/* copies data from parameter to be freed when ran. This is for passing a stack variable to queue_add */
void* queue_copy_data(const void* data, size_t size);
/* Queues event from seperate thread to be executed on the other. */
void queue_add(handle_generic_t handler, const void* data);
/* Clears and executes queue on main thread. Returns amount of events processed */
int queue_run(void);
/* Main thread to window thread is handled by sending a message to its Win32 
   message queue; therefore, there is no respective queue_run_window function */
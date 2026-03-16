/*
	debug.h ~ RL
*/

#pragma once

#include <stdlib.h>
#ifdef _DEBUG
#include <assert.h>
#define RUNTIME_ASSERT(cond) assert(cond)
#else
#define RUNTIME_ASSERT(cond) if (!(cond)) { debug_format(#cond " failed @ line %i in file " __FILE__ ", quitting\n", __LINE__); exit(-1); }
#endif

void debug_format(const char* fmt, ...);
void debug_profiler_push(void);
void debug_profiler_pop(const char* format, ...);
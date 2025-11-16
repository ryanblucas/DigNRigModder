/*
	debug.c ~ RL
*/

#include "debug.h"
#include <stdlib.h>
#include <strsafe.h>
#include <Windows.h>

void debug_format(const char* fmt, ...)
{
	static char* current_buffer = NULL;
	static int size;

	if (!current_buffer)
	{
		size = 256;
		current_buffer = malloc(size * sizeof * current_buffer);
		if (!current_buffer)
		{
			exit(-10); /* The program most certainly couldn't run if it can't allocate 256 bytes */
		}
	}

	va_list list;
	va_start(list, fmt);
	while (StringCbVPrintfA(current_buffer, size, fmt, list) == STRSAFE_E_INSUFFICIENT_BUFFER)
	{
		free(current_buffer);
		size *= 2;
		current_buffer = malloc(size * sizeof * current_buffer);
		if (!current_buffer)
		{
			exit(-10);
		}
	}
	va_end(list);

	OutputDebugStringA(current_buffer);
}
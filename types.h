/*
	types.h ~ RL
*/

#pragma once

#include "debug.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define TARGET_WIDTH	150
#define TARGET_HEIGHT	100
#define TARGET_CELL_SIZE 8

#define CREATE_ATTRIBUTE(fg, bg) ((fg) | (bg) << 4)

/* 4-bit color, 0bIRGB */
typedef enum color
{
	DARK_BLACK,
	DARK_BLUE,
	DARK_GREEN,
	DARK_AQUA,
	DARK_RED,
	DARK_PURPLE,
	DARK_YELLOW,
	LIGHT_GRAY,
	DARK_GRAY,
	LIGHT_BLUE,
	LIGHT_GREEN,
	LIGHT_AQUA,
	LIGHT_RED,
	LIGHT_PURPLE,
	LIGHT_YELLOW,
	LIGHT_WHITE
} color_t;

typedef uint16_t attribute_t;

typedef struct sprite* sprite_t;

extern inline void* dig_malloc(size_t size)
{
	void* res = malloc(size);
	if (!res)
	{
		exit(-10);
	}
	return res;
}
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

#define DNR_DIRT_INDEX 6
#define DNR_DEFAULT_DIRT_COLOR 0x4175A4
#define DNR_FONT L"digfont9"
#define DNR_FONT_A "digfont9"

#define CREATE_ATTRIBUTE(fg, bg) ((fg) | (bg) << 4)
#define ATTRIBUTE_FOREGROUND(attrib) ((attrib) & 0x0F)
#define ATTRIBUTE_BACKGROUND(attrib) (((attrib) >> 4) & 0x0F)

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

/* defined in screen.c */
extern const uint32_t palette[16];

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
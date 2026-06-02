/*
	types.h ~ RL
*/

#pragma once

#include "debug.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define RAISE_EVENT(ev, ...) if (ev) ev(__VA_ARGS__);

#define TARGET_WIDTH	150
#define TARGET_HEIGHT	100
#define TARGET_CELL_SIZE 8
#define TARGET_CELL_SIZE_SMALL 6

#define LAYER_COUNT 14

#define WORLD_WIDTH TARGET_WIDTH
#define WORLD_HEIGHT (TARGET_HEIGHT * LAYER_COUNT)

#define DNR_DIRT_INDEX 6
#define DNR_DEFAULT_DIRT_COLOR 0x4175A4
#define DNR_FONT L"digfont9"
#define DNR_FONT_A "digfont9"
#define DNR_FONT_SMALL L"digfont9small"
#define DNR_FONT_SMALL_A "digfont9small"

#define CREATE_ATTRIBUTE(fg, bg) ((fg) | (bg) << 4)
#define ATTRIBUTE_FOREGROUND(attrib) ((attrib) & 0x0F)
#define ATTRIBUTE_BACKGROUND(attrib) (((attrib) >> 4) & 0x0F)

typedef enum info_mode
{
	MODE_SAVE,
	MODE_ASSET,
	// MODE_SPRITE,
	MODE_COUNT
} info_mode_t;

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

typedef uint32_t rgb_color_t;
/* defined in screen.c */
extern const rgb_color_t palette[16];

typedef uint16_t attribute_t;

typedef struct region
{
	int x0, y0, x1, y1;
} region_t;

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

extern inline bool dig_inside_bounds(int x, int y)
{
	return x >= 0 && y >= 0 && x < WORLD_WIDTH && y < WORLD_HEIGHT;
}

/* not a fan... */
#define INVALID_REGION ((region_t) { INT_MIN, INT_MIN, INT_MIN, INT_MIN })

extern inline int region_width(region_t region)
{
	return abs(region.x1 - region.x0) + 1;
}

extern inline int region_height(region_t region)
{
	return abs(region.y1 - region.y0) + 1;
}

extern inline int region_size(region_t region)
{
	return region_width(region) * region_height(region);
}

extern inline region_t region_validate(region_t region)
{
	if (region.x1 < region.x0)
	{
		int temp = region.x0;
		region.x0 = region.x1;
		region.x1 = temp;
	}
	if (region.y1 < region.y0)
	{
		int temp = region.y0;
		region.y0 = region.y1;
		region.y1 = temp;
	}
	return region;
}

extern inline bool region_is_invalid(region_t region)
{
	region_t invalid = INVALID_REGION;
	return region.x0 == invalid.x0 && region.y0 == invalid.y0 && region.x1 == invalid.x1 && region.y1 == invalid.y1;
}

extern inline bool region_is_inside(region_t region, int x, int y)
{
	region = region_validate(region);
	return region.x0 <= x && region.x1 >= x && region.y0 <= y && region.y1 >= y;
}

extern inline region_t region_merge(region_t region1, region_t region2)
{
	region1 = region_validate(region1);
	region2 = region_validate(region2);
	return (region_t) { min(region1.x0, region2.x0), min(region1.y0, region2.y0), max(region1.x1, region2.x1), max(region1.y1, region2.y1) };
}

extern inline region_t region_keep_inside(region_t r1, region_t r2)
{
	r1.x0 = max(r1.x0, r2.x0);
	r1.y0 = max(r1.y0, r2.y0);
	r1.x1 = min(r1.x1, r2.x1);
	r1.y1 = min(r1.y1, r2.y1);
	return r1;
}
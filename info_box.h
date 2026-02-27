/*
	info_box.h ~ RL
	Window that increases interactivity with the main display
*/

#pragma once

#include "file.h"
#include "types.h"

typedef enum info_mode
{
	MODE_SAVE,
	MODE_SPRITE,
	MODE_LAYER,

	MODE_COUNT
} info_mode_t;

typedef void (*info_handle_change_mode)(info_mode_t);
typedef void (*info_handle_change_block)(int, int);
typedef void (*info_handle_change_dirt_color)(int, rgb_color_t);

typedef struct info_events
{
	info_handle_change_mode mode_handler;
	info_handle_change_block block_handler;
	info_handle_change_dirt_color dirt_color_handler;
} info_events_t;

void info_initialize(info_events_t events);
void info_destroy(void);
info_mode_t info_get_current_mode(void);

dnr_state_t* info_state_get(void);
void info_state_set(dnr_state_t* state);
void info_cell_set_current(int x, int y);
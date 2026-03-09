/*
	info_box.h ~ RL
	Window that increases interactivity with the main display
*/

#pragma once

#include "file.h"
#include "serialize.h"

typedef enum info_mode
{
	MODE_SAVE,
	MODE_SPRITE,
	MODE_LAYER,

	MODE_COUNT
} info_mode_t;

typedef void (*info_handle_change_mode)(info_mode_t new_mode);
typedef void (*info_handle_change_block)(int x, int y);
typedef void (*info_handle_change_global_field)(const void* field);

typedef struct info_events
{
	info_handle_change_mode mode_handler;
	info_handle_change_block block_handler;
	info_handle_change_global_field global_field_handler;
} info_events_t;

void info_initialize(info_events_t events);
void info_destroy(void);
info_mode_t info_get_current_mode(void);

dnr_state_t* info_state_get(void);
void info_state_set(dnr_state_t* state);

void info_cell_set_current(int x, int y);
void info_cell_set_current_region(region_t region);

element_t info_element_find(bool global, const char* query);
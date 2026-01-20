/*
	info_box.h ~ RL
	Window that increases interactivity with the main display
*/

#pragma once

#include "types.h"

typedef enum info_mode
{
	MODE_SAVE,
	MODE_SPRITE,
	MODE_LAYER,

	MODE_COUNT
} info_mode_t;

typedef void (*info_handle_change_mode)(info_mode_t);

void info_initialize(info_handle_change_mode handler);
void info_destroy(void);
info_mode_t info_get_current_mode(void);

void info_set_current_cell(char character, attribute_t attrib, uint32_t dirt_color);
void info_set_current_message(const char* msg);
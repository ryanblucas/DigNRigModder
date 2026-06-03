/*
	save_main.h ~ RL
	Reads save files
*/

#pragma once

#include "../file.h"

void save_initialize(editor_state_t* state);
void save_destroy(void);
void save_start(void);
void save_end(void);

/* field is an offset of complete_block_t */
bool save_can_change_local_field(region_t selection, size_t field);
/* field is an offset of complete_block_t */
bool save_can_change_brush_field(size_t field);
bool save_can_change_global_field(const void* field);
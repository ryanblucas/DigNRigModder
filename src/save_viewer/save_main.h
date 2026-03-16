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
/*
	address.h ~ RL
	Hard-coded addresses in the PE and game values
*/

#pragma once

#include "file.h"

#define ADDRESS_CALL_GAME_MAKE_STARTING_RIG 0x000045C4

void address_initialize(void);

/* changes CALL opcode @ "Dig-N-Rig.exe"+addr' parameter to call whatever ptr is */
void address_change_call(uintptr_t addr, uintptr_t func);

/* Gets the file name of the layer that is loaded from game_initialize_layers by index 0-(LAYER_COUNT - 1) */
const char* address_layer_filename_get(int index);
/* Sets the file name of the layer that is loaded to game_initialize_layers by index 0-(LAYER_COUNT - 1). */
void address_layer_filename_set(int index, const char* name);
/* Gets the name of the layer as seen in game from the function that shows it by index 0-(LAYER_COUNT - 1) */
const char* address_layer_name_get(int index);
/* Sets the name of the layer as seen in game to the function that shows it by index 0-(LAYER_COUNT - 1) */
void address_layer_name_set(int index, const char* name);
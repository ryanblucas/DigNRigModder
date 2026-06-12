/*
	address.h ~ RL
	Hard-coded addresses in the PE and game values
*/

#pragma once

#define LAYER_COUNT 14

void address_initialize(void);

/* Gets the file name of the layer that is loaded from game_initialize_layers by index 0-(LAYER_COUNT - 1) */
const char* address_layer_filename_get(int index);
/* Sets the file name of the layer that is loaded to game_initialize_layers by index 0-(LAYER_COUNT - 1). */
void address_layer_filename_set(int index, const char* name);

/* Gets the name of the layer as seen in game from the function that shows it by index 0-(LAYER_COUNT - 1) */
const char* address_layer_name_get(int index);
/* Sets the name of the layer as seen in game to the function that shows it by index 0-(LAYER_COUNT - 1) */
void address_layer_name_set(int index, const char* name);
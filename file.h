/*
	file.h ~ RL
*/

#pragma once

#include "types.h"

#define SAVE_BLOCK_STRUCT_SIZE 0x54
#define SAVE_MINERAL_STRUCT_SIZE 0x34
#define SAVE_MINERAL_MAX_COUNT 0xC350

typedef struct save
{
	float x_spawn, y_spawn;
	int layer_count;
	sprite_t layer_images[];
} save_t;

sprite_t file_load_sprite(const char* directory);
save_t* file_load_save(const char* directory);
void file_unload_save(save_t* save);
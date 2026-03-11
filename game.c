/*
	game.c ~ RL
	Handles with game logic, like deleting blocks, creating minerals and stalactites, etc..
*/

#include "game.h"

#define CHECK_CONDITION(condition) if (!(condition)) { debug_format(#condition " failed at (%i, %i)\n", x, y); result = false; }

bool game_is_valid(dnr_state_t* state)
{
	bool result = true;
	for (int y = 0; y < WORLD_HEIGHT; y++)
	{
		for (int x = 0; x < WORLD_WIDTH; x++)
		{
			dnr_block_t* curr = game_get_block(state, x, y);
			CHECK_CONDITION(curr->x == x && curr->y == y);
			dnr_mineral_t* mineral = game_get_mineral(state, x, y);
			if (mineral)
			{
				CHECK_CONDITION(curr->mineral_index >= 0 && curr->mineral_index < sizeof state->minerals / sizeof * state->minerals);
			}
		}
	}
	return result;
}

void game_copy(const dnr_state_t* state, region_t region, complete_block_t* arr)
{
	region = region_validate(region);
	memset(arr, 0, region_size(region) * sizeof * arr);
	for (int y = 0; y < region_height(region); y++)
	{
		for (int x = 0; x < region_width(region); x++)
		{
			int index = y * region_width(region) + x;
			int state_index = (region.x0 + x) * WORLD_HEIGHT + (region.y0 + y);
			arr[index].block = state->blocks[state_index];
			if (state->blocks[state_index].mineral_exists)
			{
				RUNTIME_ASSERT(state->blocks[state_index].mineral_index >= 0 && state->blocks[state_index].mineral_index < sizeof state->minerals / sizeof * state->minerals);
				arr[index].mineral = state->minerals[state->blocks[state_index].mineral_index];
			}
			for (int i = 0; i < state->stalactite_count; i++)
			{
				if (state->stalactite_array[i].exists && (int)state->stalactite_array[i].x == region.x0 + x && (int)state->stalactite_array[i].y == region.y0 + y)
				{
					arr[index].stalactite = state->stalactite_array[i];
					break;
				}
			}
		}
	}
}

void game_paste(dnr_state_t* state, region_t region, const complete_block_t* arr)
{
	region = region_validate(region);
	for (int y = 0; y < region_height(region); y++)
	{
		for (int x = 0; x < region_width(region); x++)
		{
			int index = y * region_width(region) + x;
			int state_index = (region.x0 + x) * WORLD_HEIGHT + (region.y0 + y);
			
			state->blocks[state_index] = arr[index].block;
			state->blocks[state_index].x = region.x0 + x;
			state->blocks[state_index].y = region.y0 + y;
			state->blocks[state_index].layer_index = state->blocks[state_index].y / TARGET_HEIGHT;

			if (arr[index].block.mineral_exists)
			{
				dnr_mineral_t copy = arr[index].mineral;
				copy.x = copy.x - (int)copy.x + region.x0 + x;
				copy.y = copy.y - (int)copy.y + region.y0 + y;
				game_add_mineral(state, &copy);
				state->blocks[state_index].mineral_index = copy.index;
			}
			if (arr[index].stalactite.exists)
			{
				stalactite_t copy = arr[index].stalactite;
				copy.x = copy.x - (int)copy.x + region.x0 + x;
				copy.y = copy.y - (int)copy.y + region.y0 + y;
				game_add_stalactite(state, copy);
			}
		}
	}
}

void game_delete(dnr_state_t* state, region_t region)
{
	for (int x = region.x0; x <= region.x1; x++)
	{
		for (int y = region.y0; y <= region.y1; y++)
		{
			game_delete_block(state, x, y);
		}
	}
}

bool game_add_mineral(dnr_state_t* state, dnr_mineral_t* mineral)
{
	for (int i = 0; i < sizeof state->minerals / sizeof * state->minerals; i++)
	{
		if (!state->minerals[i].exists)
		{
			state->minerals[i] = *mineral;
			mineral->index = i;
			return true;
		}
	}
	return false;
}

bool game_add_stalactite(dnr_state_t* state, stalactite_t stalactite)
{
	for (int i = 0; i < state->stalactite_count; i++)
	{
		if (!state->stalactite_array[i].exists)
		{
			state->stalactite_array[i] = stalactite;
			return true;
		}
	}
	return false;
}

dnr_block_t* game_get_block(dnr_state_t* state, int x, int y)
{
	RUNTIME_ASSERT(dig_inside_bounds(x, y));
	return &state->blocks[y + x * WORLD_HEIGHT];
}

dnr_mineral_t* game_get_mineral(dnr_state_t* state, int x, int y)
{
	RUNTIME_ASSERT(dig_inside_bounds(x, y));
	dnr_block_t* block = game_get_block(state, x, y);
	if (block->mineral_exists && block->mineral_index >= 0 && block->mineral_index < sizeof state->minerals / sizeof * state->minerals)
	{
		return &state->minerals[block->mineral_index];
	}
	block->mineral_exists = false;
	block->mineral_index = -1;
	return NULL;
}

stalactite_t* game_get_stalactite(dnr_state_t* state, int x, int y)
{
	RUNTIME_ASSERT(dig_inside_bounds(x, y));
	for (int i = 0; i < state->stalactite_count; i++)
	{
		if (state->stalactite_array[i].exists && (int)state->stalactite_array[i].x == x && (int)state->stalactite_array[i].y == y)
		{
			return &state->stalactite_array[i];
		}
	}
	return NULL;
}

void game_delete_block(dnr_state_t* state, int x, int y)
{
	dnr_mineral_t* mineral = game_get_mineral(state, x, y);
	stalactite_t* stalactite = game_get_stalactite(state, x, y);
	game_delete_block_partial(state, x, y);
	if (mineral)
	{
		mineral->exists = false;
	}
	if (stalactite)
	{
		stalactite->exists = false;
	}
}

void game_delete_block_partial(dnr_state_t* state, int x, int y)
{
	dnr_block_t* block = game_get_block(state, x, y);
	if (!block)
	{
		return;
	}
	block->health_current = 0;
	block->visual.Attributes = 0;
	block->visual.Char.AsciiChar = ' ';
	block->block_exists = false;
	block->can_mine = true;
	block->mineral_exists = false;
	block->rig_type = RIG_NONE;
	block->mineral_move_direction = MOVE_DIRECTION_DOWN;
}
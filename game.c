/*
	game.c ~ RL
	Handles with game logic, like deleting blocks, creating minerals and stalactites, etc..
*/

#include "game.h"

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
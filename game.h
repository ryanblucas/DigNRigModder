/*
	game.h ~ RL
	Handles with game logic, like deleting blocks, creating minerals and stalactites, etc..
*/

#pragma once

#include "file.h"

/* Determines if a game state is valid */
bool game_is_valid(dnr_state_t* state);

/* Returns the block at a position */
dnr_block_t* game_get_block(dnr_state_t* state, int x, int y);
/* Returns the mineral at a position */
dnr_mineral_t* game_get_mineral(dnr_state_t* state, int x, int y);
/* Returns the stalactite at a position */
stalactite_t* game_get_stalactite(dnr_state_t* state, int x, int y);
/* Deletes everything at a block, including the mineral and stalactite */
void game_delete_block(dnr_state_t* state, int x, int y);
/* Deletes the block, but not a mineral or stalactite there */
void game_delete_block_partial(dnr_state_t* state, int x, int y);
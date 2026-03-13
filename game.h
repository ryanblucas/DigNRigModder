/*
	game.h ~ RL
	Handles with game logic, like deleting blocks, creating minerals and stalactites, etc..
*/

#pragma once

#include "file.h"
#include <Windows.h>

typedef struct complete_block
{
	dnr_block_t block;
	dnr_mineral_t mineral;
	stalactite_t stalactite;
} complete_block_t;

/* Determines if a game state is valid */
bool game_is_valid(const dnr_state_t* state);

/* copies a region from state into arr in region */
void game_copy(const dnr_state_t* state, region_t region, complete_block_t* arr);
/* pastes a region from state into arr in region */
void game_paste(dnr_state_t* state, region_t region, const complete_block_t* arr);
/* deletes a region from state */
void game_delete(dnr_state_t* state, region_t region);

/* Finds spot to put mineral, otherwise return false. Sets the index parameter in mineral to the open spot */
bool game_add_mineral(dnr_state_t* state, dnr_mineral_t* mineral);
/* Finds spot to put stalactite, otherwise return false */
bool game_add_stalactite(dnr_state_t* state, stalactite_t stalactite);

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

/* Renders a block */
CHAR_INFO game_spritify_cell(const complete_block_t* block);
/* Renders a layer */
sprite_t game_spritify_layer(const dnr_state_t* save, int layer_index);
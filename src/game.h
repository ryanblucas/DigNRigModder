/*
	game.h ~ RL
	Handles with game logic, like deleting blocks, creating minerals and stalactites, etc..
*/

#pragma once

#include "file.h"
#include <Windows.h>

#define GAME_BLANK_CHAR 0xDB
#define GAME_STONE_CHAR 0xB0
#define GAME_BRICK_CHAR 0xB1

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

/* Renders a block */
CHAR_INFO game_spritify_cell(const complete_block_t* block);
/* Renders a layer */
sprite_t game_spritify_layer(const dnr_state_t* save, int layer_index);
/* Renders an asset */
sprite_t game_spritify_asset(const asset_t asset);

/* copies a region from asset into arr in region */
void game_asset_copy(const asset_t* state, region_t region, asset_block_t* arr);
/* pastes a region from asset into arr in region */
void game_asset_paste(asset_t* state, region_t region, const asset_block_t* arr);
/* deletes a region from asset */
void game_asset_delete(asset_t* state, region_t region);
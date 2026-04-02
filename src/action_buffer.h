/*
	action_buffer.h ~ RL
	Buffers actions for undo/redo functionality
*/

#pragma once

#include "file.h"
#include "game.h"
#include "serialize.h"

typedef struct complete_field
{
	size_t field_size;
	void* value;
} complete_field_t;

typedef enum action_type
{
	ACTION_BLOCK,
	ACTION_FIELD,
	ACTION_ASSET_BLOCK,
} action_type_t;

/* Dig-N-Rig is a 32-bit application, meaning all elementary fields (like CHAR_INFO 
   and ints, not something like dnr_save_header_t) are at least 4 bytes. */
typedef uint32_t field_t;

typedef struct action_block
{
	region_t region;
	complete_block_t* previous;
	complete_block_t* next;
} action_block_t;

typedef struct action_field
{
	element_t element;
	field_t previous;
	field_t next;
} action_field_t;

typedef struct action_asset_block
{
	region_t region;
	asset_block_t* previous;
	asset_block_t* next;
} action_asset_block_t;

typedef struct action
{
	action_type_t type;
	union
	{
		action_block_t block;
		action_field_t field;
		action_asset_block_t asset;
	} sub;
} action_t;

extern inline field_t field_create(const void* ptr, size_t size)
{
	field_t res;
	RUNTIME_ASSERT(size <= sizeof res);
	if (size == 4)
	{
		res = *(uint32_t*)ptr;
	}
	else if (size == 2)
	{
		res = *(uint16_t*)ptr;
	}
	else
	{
		res = *(uint8_t*)ptr;
	}
	return res;
}

typedef struct action_buffer* action_buffer_t;

action_buffer_t action_buffer_initialize(void);
void action_buffer_destroy(action_buffer_t buffer);

/* Adds block action to buffer. The action created uses the region parameter passed in
   and creates the "previous" array with what is there currently. Therefore, call this
   before changing anything, then action_buffer_post_add_block to create the next part.
   If this isn't the top of the buffer, it deletes everything in front of it */
void action_buffer_pre_add_block(action_buffer_t buffer, const dnr_state_t* state, region_t region);
/* Finalizes block action to buffer from pre_add_block. */
void action_buffer_post_add_block(action_buffer_t buffer, const dnr_state_t* state);
/* Adds block action to buffer. */
void action_buffer_add_block(action_buffer_t buffer, const complete_block_t* prev, const complete_block_t* curr, region_t region);

/* Adds field action to buffer. All of the parameters passed in are 1:1 what is created
   for the action_field_t struct. If this isn't the top of the buffer, it deletes everything in front of it */
void action_buffer_add_field(action_buffer_t buffer, element_t element, field_t previous);

/* Adds asset block action to buffer. The action created uses the region parameter passed in
   and creates the "previous" array with what is there currently. Therefore, call this
   before changing anything, then action_buffer_post_add_asset_block to create the next part.
   If this isn't the top of the buffer, it deletes everything in front of it */
void action_buffer_pre_add_asset_block(action_buffer_t buffer, const asset_t* asset, region_t region);
/* Finalizes asset block action to buffer from pre_add_block. */
void action_buffer_post_add_asset_block(action_buffer_t buffer, const asset_t* asset);
/* Adds asset block action to buffer. */
void action_buffer_add_asset_block(action_buffer_t buffer, const asset_block_t* prev, const asset_block_t* curr, region_t region);

/* Goes back in the buffer. If there's no more left, returns false and doesn't write to action.
   This would be used for undoing */
action_t* action_buffer_back(action_buffer_t buffer);
/* Goes forward in the buffer. If there's no more left, returns false and doesn't write to action.
   This would be used for redoing */
action_t* action_buffer_forward(action_buffer_t buffer);
/* Reverses a block action */
void action_buffer_reverse_block(dnr_state_t* state, action_t* action);
/* Reverses a field action */
void action_buffer_reverse_field(action_t* action);
/* Reverses a asset block action */
void action_buffer_reverse_asset_block(asset_t* asset, action_t* action);